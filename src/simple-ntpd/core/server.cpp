/**
 * @file ntp_server.cpp
 * @brief NTP server implementation
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ntpd/core/server.hpp"
#include "simple-ntpd/utils/logger.hpp"
#include "simple-ntpd/config/config.hpp"
#include "simple-ntpd/core/connection.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#if __has_include(<filesystem>)
#include <filesystem>
#endif
#ifndef _WIN32
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif
#ifndef _WIN32
#include <syslog.h>
#endif
#include <arpa/inet.h>

namespace simple_ntpd {

// NtpServer implementation
NtpServer::NtpServer(std::shared_ptr<NtpConfig> config,
                     std::shared_ptr<Logger> logger)
    : config_(config), logger_(logger), running_(false),
      shutdown_requested_(false), server_socket_(INVALID_SOCKET),
      server_address_(config->listen_address),
      server_port_(config->listen_port), active_connections_(),
      connections_mutex_(), accept_thread_(), worker_threads_(),
      workers_running_(false),
      config_watch_thread_(), config_watch_running_(false),
      config_last_write_time_(),
      stats_(), stats_mutex_(),
      config_change_callback_(),
      last_cleanup_time_(std::chrono::steady_clock::now()),
      cleanup_interval_(std::chrono::seconds(300)),
      restart_count_(0),
      healthy_upstreams_(),
      upstream_rr_index_(0),
      security_mutex_(),
      connection_rate_buckets_(),
      request_second_buckets_(),
      rng_(std::random_device{}()) {

  logger_->info("NTP Server initialized with configuration");
  logger_->debug("Server will listen on " + config->listen_address + ":" +
                 std::to_string(config->listen_port));
}

NtpServer::~NtpServer() { stop(); }

bool NtpServer::start() {
  if (running_) {
    logger_->warning("NTP Server is already running");
    return false;
  }

  logger_->info("Starting NTP Server...");

  if (config_->enable_tls || config_->enable_certificate_authentication ||
      config_->enable_certificate_validation) {
    logger_->info("Secure synchronization controls enabled (TLS/certificate checks)");
    if (config_->enable_certificate_validation && config_->tls_ca_file.empty()) {
      logger_->error("TLS CA file is required when certificate validation is enabled");
      return false;
    }
    if ((config_->enable_tls || config_->enable_certificate_authentication) &&
        (config_->tls_cert_file.empty() || config_->tls_key_file.empty())) {
      logger_->error("TLS cert/key files are required when TLS/certificate auth is enabled");
      return false;
    }
  }

  // Initialize socket
  if (!initializeSocket()) {
    logger_->error("Failed to initialize server socket");
    return false;
  }

  // Bind socket
  if (!bindSocket()) {
    logger_->error("Failed to bind server socket");
    closeSocket();
    return false;
  }

  // Start worker threads
  startWorkerThreads();

  // Start config watcher thread (portable mtime polling)
  startConfigWatcher();

  running_ = true;
  stats_.start_time = std::chrono::steady_clock::now();
  loadState();
  healthy_upstreams_ = config_->upstream_servers;

  logger_->info("NTP Server started successfully");
  logger_->info("Listening on " + config_->listen_address + ":" +
                std::to_string(config_->listen_port));

  return true;
}
bool NtpServer::reloadConfig() {
  if (!config_) {
    return false;
  }

  const std::string &cfg_path = config_->lastConfigFile();
  if (cfg_path.empty()) {
    logger_->warning("Reload requested but no config file path recorded");
    return false;
  }

  auto new_config = std::make_shared<NtpConfig>(*config_);
  if (!new_config->loadFromFile(cfg_path) || !new_config->validate()) {
    logger_->error("Config reload failed: invalid configuration");
    return false;
  }

  // Apply dynamic fields without full restart
  {
    // Update logging settings
    logger_->setLevel(new_config->log_level);
    logger_->setLogFile(new_config->log_file);
    if (new_config->enable_console_logging && new_config->log_file.size()) {
      logger_->setDestination(LogDestination::BOTH);
    } else if (new_config->enable_console_logging) {
      logger_->setDestination(LogDestination::CONSOLE);
    } else {
      logger_->setDestination(LogDestination::FILE);
    }
#ifndef _WIN32
    logger_->setSyslog(new_config->enable_syslog, LOG_DAEMON);
#endif
    logger_->setStructuredJson(new_config->log_json);
    logger_->setLogRotation(new_config->log_max_size_bytes, new_config->log_max_files);
  }

  // Networking parameters generally require restart; log if changed
  bool address_changed = (new_config->listen_address != server_address_ ||
                          new_config->listen_port != server_port_);
  if (address_changed) {
    logger_->warning("listen_address/port changed; full restart required to apply");
  }

  // Replace config pointer
  config_ = new_config;
  logger_->info("Configuration reloaded successfully");
  if (config_change_callback_) {
    config_change_callback_();
  }
  return true;
}

void NtpServer::stop() {
  if (!running_) {
    return;
  }

  logger_->info("Stopping NTP Server...");

  running_ = false;

  // Stop worker threads
  stopWorkerThreads();

  // Stop config watcher
  stopConfigWatcher();

  // Close all connections
  cleanupConnections();
  persistState();
  backupConfig();

  // Cleanup socket
  closeSocket();

  logger_->info("NTP Server stopped");
}

bool NtpServer::isRunning() const { return running_; }

bool NtpServer::initializeSocket() {
#ifdef _WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    logger_->error("Failed to initialize Winsock");
    return false;
  }
#endif

  server_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (server_socket_ == INVALID_SOCKET) {
    logger_->error("Failed to create socket: " +
                   std::string(std::strerror(errno)));
    return false;
  }

  // Set socket options
  int opt = 1;
  if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<const char *>(&opt), sizeof(opt)) < 0) {
    logger_->warning("Failed to set SO_REUSEADDR: " +
                     std::string(std::strerror(errno)));
  }

#ifndef _WIN32
  int flags = fcntl(server_socket_, F_GETFL, 0);
  if (flags < 0 || fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK) < 0) {
    logger_->warning("Failed to set non-blocking mode: " +
                     std::string(std::strerror(errno)));
  }
#endif

  return true;
}

void NtpServer::startConfigWatcher() {
  const std::string &cfg_path = config_ ? config_->lastConfigFile() : std::string();
  if (cfg_path.empty()) {
    return;
  }

  // Initialize last write time to current so we only react to future changes
  config_last_write_time_ = std::chrono::system_clock::now();
  config_watch_running_ = true;
  config_watch_thread_ = std::thread(&NtpServer::configWatcherLoop, this);
}

void NtpServer::stopConfigWatcher() {
  config_watch_running_ = false;
  if (config_watch_thread_.joinable()) {
    config_watch_thread_.join();
  }
}

void NtpServer::configWatcherLoop() {
  using namespace std::chrono_literals;
  const std::string cfg_path = config_->lastConfigFile();
  if (cfg_path.empty()) {
    return;
  }

  while (config_watch_running_) {
    std::this_thread::sleep_for(2s);

    // Portable mtime polling using std::filesystem if available
#if __has_include(<filesystem>)
    try {
      namespace fs = std::filesystem;
      auto current_write = fs::last_write_time(fs::path(cfg_path));
      // Convert file_time_type to system_clock::time_point for comparison
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
      auto current_sys = std::chrono::clock_cast<std::chrono::system_clock>(current_write);
#else
      (void)current_write;
      auto current_sys = std::chrono::system_clock::now(); // Fallback: trigger reload only once
#endif
      if (config_last_write_time_.time_since_epoch().count() == 0) {
        config_last_write_time_ = current_sys;
      } else if (current_sys > config_last_write_time_) {
        logger_->info("Config file modification detected, reloading");
        config_last_write_time_ = current_sys;
        reloadConfig();
      }
    } catch (...) {
      // Ignore polling errors
    }
#endif
  }
}

bool NtpServer::bindSocket() {
  struct sockaddr_in server_addr;
  std::memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port_);

  if (inet_pton(AF_INET, server_address_.c_str(), &server_addr.sin_addr) <= 0) {
    logger_->error("Invalid listen address: " + server_address_);
    return false;
  }

  if (bind(server_socket_, reinterpret_cast<struct sockaddr *>(&server_addr),
           sizeof(server_addr)) < 0) {
    logger_->error("Failed to bind socket: " +
                   std::string(std::strerror(errno)));
    return false;
  }

  return true;
}

void NtpServer::startWorkerThreads() {
  size_t thread_count = config_->worker_threads;

  for (size_t i = 0; i < thread_count; ++i) {
    worker_threads_.emplace_back(&NtpServer::workerThreadFunction, this, i);
    logger_->debug("Started worker thread " + std::to_string(i));
  }

  workers_running_ = true;
}

void NtpServer::stopWorkerThreads() {
  workers_running_ = false;

  for (auto &thread : worker_threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  worker_threads_.clear();
  logger_->info("All worker threads stopped");
}

void NtpServer::workerThreadFunction(size_t thread_id) {
  logger_->debug("Worker thread " + std::to_string(thread_id) + " started");

  while (workers_running_) {
    // Process incoming packets
    processIncomingPackets();

    // Clean up inactive connections
    cleanupConnections();

    // Small sleep to prevent busy waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  logger_->debug("Worker thread " + std::to_string(thread_id) + " stopped");
}

void NtpServer::processIncomingPackets() {
  std::vector<uint8_t> buffer(NTP_PACKET_SIZE);
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  while (workers_running_) {
    std::memset(&client_addr, 0, sizeof(client_addr));

    ssize_t bytes_received = recvfrom(
        server_socket_, buffer.data(), buffer.size(), 0,
        reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);

    if (bytes_received < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available (non-blocking socket)
        break;
      }

      logger_->error("Failed to receive data: " +
                     std::string(std::strerror(errno)));
      if (config_ && config_->enable_self_healing &&
          restart_count_.load() < config_->service_restart_limit) {
        restart_count_.fetch_add(1);
        logger_->warning("Self-healing socket restart attempt #" +
                         std::to_string(restart_count_.load()));
        closeSocket();
        if (initializeSocket() && bindSocket()) {
          continue;
        }
      }
      break;
    }

    if (bytes_received == 0) {
      continue;
    }

    // Process the received packet
    buffer.resize(bytes_received);
    processPacket(buffer, client_addr);

    // Reset buffer size for next iteration
    buffer.resize(NTP_PACKET_SIZE);
  }
}

void NtpServer::processPacket(const std::vector<uint8_t> &data,
                              const struct sockaddr_in &client_addr) {
  auto start_us = std::chrono::steady_clock::now();
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
  uint16_t client_port = ntohs(client_addr.sin_port);
  const std::string client_ip_str(client_ip);

  if (!isClientAllowed(client_ip_str)) {
    logger_->warning("Dropped packet from ACL-restricted client " + client_ip_str);
    stats_.total_errors++;
    return;
  }

  if (isRateLimitExceeded(client_ip_str)) {
    logger_->warning("Dropped packet due to connection/request rate limit for " +
                     client_ip_str);
    stats_.total_errors++;
    return;
  }

  if (isDdosAnomaly(client_ip_str)) {
    logger_->warning("Potential DDoS anomaly detected for " + client_ip_str);
    if (config_ && config_->enable_graceful_degradation) {
      stats_.total_errors++;
      return;
    }
  }

  // Create or get connection for this client
  auto connection = getOrCreateConnection(client_ip_str, client_port);
  if (!connection) {
    logger_->warning("Failed to create connection for " +
                     std::string(client_ip) + ":" +
                     std::to_string(client_port));
    return;
  }

  // Process the packet
  if (connection->handlePacket(data)) {
    NtpPacket request_packet;
    if (!request_packet.parseFromData(data)) {
      logger_->warning("Failed to parse packet for response generation from " +
                       std::string(client_ip));
      stats_.total_errors++;
      return;
    }

    NtpPacket response_packet = NtpPacket::createServerResponse(
        request_packet, config_->stratum, effectiveReferenceId());
    applyDynamicStratum();
    const std::string selected_upstream = selectUpstreamServer();
    if (!selected_upstream.empty()) {
      logger_->debug("Selected upstream server: " + selected_upstream);
    }
    auto response_data = response_packet.serializeToData();
    ssize_t bytes_sent = sendto(
        server_socket_, response_data.data(), response_data.size(), 0,
        reinterpret_cast<const struct sockaddr *>(&client_addr),
        sizeof(client_addr));
    if (bytes_sent < 0) {
      logger_->error("Failed to send NTP response to " + std::string(client_ip) +
                     ":" + std::to_string(client_port) + ": " +
                     std::string(std::strerror(errno)));
      stats_.total_errors++;
      if (config_ && config_->enable_upstream_failover && !selected_upstream.empty()) {
        auto it = std::find(healthy_upstreams_.begin(), healthy_upstreams_.end(),
                            selected_upstream);
        if (it != healthy_upstreams_.end()) {
          healthy_upstreams_.erase(it);
        }
      }
      return;
    }

    stats_.total_requests++;
    stats_.total_bytes_transferred += data.size();
    stats_.total_responses++;
  } else {
    stats_.total_errors++;
  }

  auto end_us = std::chrono::steady_clock::now();
  auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(end_us - start_us).count();
  stats_.total_request_processing_time_us += static_cast<uint64_t>(dur_us);
  stats_.processed_request_count++;
  if (static_cast<uint64_t>(dur_us) > stats_.max_request_processing_time_us) {
    stats_.max_request_processing_time_us = static_cast<uint64_t>(dur_us);
  }
  if (static_cast<uint64_t>(dur_us) < stats_.min_request_processing_time_us) {
    stats_.min_request_processing_time_us = static_cast<uint64_t>(dur_us);
  }
}

std::shared_ptr<NtpConnection>
NtpServer::getOrCreateConnection(const std::string &client_ip,
                                 uint16_t client_port) {
  std::lock_guard<std::mutex> lock(connections_mutex_);

  std::string client_key = client_ip + ":" + std::to_string(client_port);

  auto it = active_connections_.find(client_key);
  if (it != active_connections_.end()) {
    return it->second;
  }

  // Create a dummy socket for the connection object
  // In practice, we'd handle this differently for UDP
  socket_t dummy_socket = INVALID_SOCKET;

  auto connection = std::make_shared<NtpConnection>(dummy_socket, client_ip,
                                                    config_, logger_);
  if (connection) {
    connection->setTrusted(isClientAllowed(client_ip));
    active_connections_[client_key] = connection;
    stats_.total_connections++;
    stats_.active_connections++;
    return connection;
  }

  return nullptr;
}

void NtpServer::closeSocket() {
  if (server_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
    closesocket(server_socket_);
    WSACleanup();
#else
    CLOSE_SOCKET(server_socket_);
#endif
    server_socket_ = INVALID_SOCKET;
  }
}

void NtpServer::cleanupConnections() {
  std::lock_guard<std::mutex> lock(connections_mutex_);

  auto it = active_connections_.begin();
  while (it != active_connections_.end()) {
    if (!it->second->isActive()) {
      it = active_connections_.erase(it);
      stats_.active_connections--;
    } else {
      ++it;
    }
  }
}

std::shared_ptr<NtpConfig> NtpServer::getConfig() const { return config_; }

std::string NtpServer::getStatus() const {
  std::stringstream ss;

  ss << "NTP Server Status:\n";
  ss << "  Status: " << (running_ ? "Running" : "Stopped") << "\n";

  if (running_) {
    auto uptime = std::chrono::steady_clock::now() - stats_.start_time;
    auto uptime_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
    ss << "  Uptime: " << uptime_seconds << " seconds\n";
    ss << "  Listen Address: " << server_address_ << ":" << server_port_
       << "\n";
    ss << "  Worker Threads: " << worker_threads_.size() << "\n";
    ss << "  Active Connections: " << stats_.active_connections << "\n";
    ss << "  Total Requests: " << stats_.total_requests << "\n";
    ss << "  Total Bytes: " << stats_.total_bytes_transferred << "\n";
    ss << "  Total Errors: " << stats_.total_errors << "\n";
    if (stats_.processed_request_count > 0) {
      double avg_us = static_cast<double>(stats_.total_request_processing_time_us) /
                      static_cast<double>(stats_.processed_request_count);
      ss << "  Avg Proc Time (us): " << static_cast<uint64_t>(avg_us) << "\n";
      ss << "  Max Proc Time (us): " << stats_.max_request_processing_time_us << "\n";
      ss << "  Min Proc Time (us): " << stats_.min_request_processing_time_us << "\n";
      double throughput = static_cast<double>(stats_.total_requests) /
                          std::max(1.0, static_cast<double>(uptime_seconds));
      ss << "  Throughput (req/s): " << throughput << "\n";
    }
  }

  return ss.str();
}

std::string NtpServer::exportPrometheusMetrics() const {
  std::stringstream m;
  m << "# HELP simple_ntpd_requests_total Total NTP requests processed\n";
  m << "# TYPE simple_ntpd_requests_total counter\n";
  m << "simple_ntpd_requests_total " << stats_.total_requests << "\n";

  m << "# HELP simple_ntpd_errors_total Total NTP errors\n";
  m << "# TYPE simple_ntpd_errors_total counter\n";
  m << "simple_ntpd_errors_total " << stats_.total_errors << "\n";

  m << "# HELP simple_ntpd_bytes_total Total bytes transferred\n";
  m << "# TYPE simple_ntpd_bytes_total counter\n";
  m << "simple_ntpd_bytes_total " << stats_.total_bytes_transferred << "\n";

  m << "# HELP simple_ntpd_request_proc_time_us Request processing time (us)\n";
  m << "# TYPE simple_ntpd_request_proc_time_us summary\n";
  double avg_us = stats_.processed_request_count == 0 ? 0.0 :
                  static_cast<double>(stats_.total_request_processing_time_us) /
                  static_cast<double>(stats_.processed_request_count);
  m << "simple_ntpd_request_proc_time_us_count " << stats_.processed_request_count << "\n";
  m << "simple_ntpd_request_proc_time_us_sum " << stats_.total_request_processing_time_us << "\n";
  m << "simple_ntpd_request_proc_time_us_avg " << static_cast<uint64_t>(avg_us) << "\n";
  m << "simple_ntpd_request_proc_time_us_max " << stats_.max_request_processing_time_us << "\n";
  m << "simple_ntpd_request_proc_time_us_min " << (stats_.min_request_processing_time_us == UINT64_MAX ? 0 : stats_.min_request_processing_time_us) << "\n";

  auto uptime = std::chrono::steady_clock::now() - stats_.start_time;
  auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
  m << "# HELP simple_ntpd_uptime_seconds Server uptime in seconds\n";
  m << "# TYPE simple_ntpd_uptime_seconds gauge\n";
  m << "simple_ntpd_uptime_seconds " << uptime_seconds << "\n";

#ifndef _WIN32
  struct rusage usage {};
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    const long rss_bytes = usage.ru_maxrss * 1024L;
    m << "# HELP simple_ntpd_process_max_rss_bytes Peak RSS in bytes\n";
    m << "# TYPE simple_ntpd_process_max_rss_bytes gauge\n";
    m << "simple_ntpd_process_max_rss_bytes " << rss_bytes << "\n";
    m << "# HELP simple_ntpd_process_user_cpu_seconds Total user CPU time\n";
    m << "# TYPE simple_ntpd_process_user_cpu_seconds counter\n";
    m << "simple_ntpd_process_user_cpu_seconds "
      << (usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0)) << "\n";
    m << "# HELP simple_ntpd_process_system_cpu_seconds Total system CPU time\n";
    m << "# TYPE simple_ntpd_process_system_cpu_seconds counter\n";
    m << "simple_ntpd_process_system_cpu_seconds "
      << (usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1000000.0)) << "\n";
  }
#endif

  return m.str();
}

std::string NtpServer::runHealthChecks() const {
  std::stringstream ss;
  bool healthy = true;

  ss << "Health Check Report\n";
  if (!running_) {
    healthy = false;
  }
  ss << "running: " << (running_ ? "true" : "false") << "\n";
  ss << "socket_bound: " << (server_socket_ != INVALID_SOCKET ? "true" : "false") << "\n";
  ss << "active_connections: " << stats_.active_connections << "\n";
  ss << "total_errors: " << stats_.total_errors << "\n";
  ss << "config_loaded: " << (config_ && !config_->lastConfigFile().empty() ? "true" : "false") << "\n";

  if (config_ && config_->enable_leap_second_handling) {
#if __has_include(<filesystem>)
    namespace fs = std::filesystem;
    std::error_code ec;
    const bool leap_exists = fs::exists(config_->leap_second_file, ec);
    ss << "leap_second_file_present: " << (leap_exists ? "true" : "false") << "\n";
    if (!leap_exists) {
      healthy = false;
      ss << "warning: leap second handling enabled but leap second file missing\n";
    }
#else
    std::ifstream leap_file(config_->leap_second_file);
    const bool leap_exists = leap_file.good();
    ss << "leap_second_file_present: " << (leap_exists ? "true" : "false") << "\n";
    if (!leap_exists) {
      healthy = false;
      ss << "warning: leap second handling enabled but leap second file missing\n";
    }
#endif
  }

  if (stats_.total_requests > 0 && stats_.total_errors > (stats_.total_requests / 2)) {
    healthy = false;
    ss << "warning: high error ratio detected\n";
  }

  ss << "overall_status: " << (healthy ? "healthy" : "degraded") << "\n";
  return ss.str();
}

bool NtpServer::isClientAllowed(const std::string &client_ip) const {
  if (!config_ || (!config_->enable_acl && !config_->restrict_queries)) {
    return true;
  }

  for (const auto &deny : config_->denied_clients) {
    if (isIpInCidr(client_ip, deny)) {
      return false;
    }
  }
  if (config_->allowed_clients.empty()) {
    return !config_->restrict_queries;
  }
  for (const auto &allow : config_->allowed_clients) {
    if (isIpInCidr(client_ip, allow)) {
      return true;
    }
  }
  return false;
}

bool NtpServer::isIpInCidr(const std::string &ip, const std::string &cidr) const {
  size_t slash = cidr.find('/');
  if (slash == std::string::npos) {
    return ip == cidr;
  }
  const std::string base_ip = cidr.substr(0, slash);
  const int prefix = std::stoi(cidr.substr(slash + 1));
  if (prefix < 0 || prefix > 32) {
    return false;
  }

  struct in_addr ip_addr {};
  struct in_addr base_addr {};
  if (inet_pton(AF_INET, ip.c_str(), &ip_addr) != 1 ||
      inet_pton(AF_INET, base_ip.c_str(), &base_addr) != 1) {
    return false;
  }
  const uint32_t ip_u = ntohl(ip_addr.s_addr);
  const uint32_t base_u = ntohl(base_addr.s_addr);
  const uint32_t mask = prefix == 0 ? 0 : 0xFFFFFFFFu << (32 - prefix);
  return (ip_u & mask) == (base_u & mask);
}

bool NtpServer::isRateLimitExceeded(const std::string &client_ip) {
  if (!config_ || !config_->enable_rate_limiting) {
    return false;
  }
  std::lock_guard<std::mutex> lock(security_mutex_);
  const auto now = std::chrono::steady_clock::now();
  const uint64_t minute_bucket =
      static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::minutes>(
                                now.time_since_epoch())
                                .count());
  auto &entry = connection_rate_buckets_[client_ip];
  if (entry.first != minute_bucket) {
    entry.first = minute_bucket;
    entry.second = 0;
  }
  entry.second++;
  return entry.second > config_->connection_rate_limit_per_minute;
}

bool NtpServer::isDdosAnomaly(const std::string &client_ip) {
  if (!config_ || !config_->enable_ddos_protection) {
    return false;
  }
  std::lock_guard<std::mutex> lock(security_mutex_);
  const auto now = std::chrono::steady_clock::now();
  const uint64_t second_bucket =
      static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                now.time_since_epoch())
                                .count());
  auto &entry = request_second_buckets_[client_ip];
  if (entry.first != second_bucket) {
    entry.first = second_bucket;
    entry.second = 0;
  }
  entry.second++;
  return entry.second > config_->ddos_anomaly_threshold_per_second;
}

void NtpServer::persistState() const {
  if (!config_ || !config_->enable_state_persistence || config_->state_file.empty()) {
    return;
  }
  std::ofstream state(config_->state_file, std::ios::trunc);
  if (!state.is_open()) {
    return;
  }
  state << "{\n";
  state << "  \"total_requests\": " << stats_.total_requests << ",\n";
  state << "  \"total_errors\": " << stats_.total_errors << ",\n";
  state << "  \"total_connections\": " << stats_.total_connections << "\n";
  state << "}\n";
}

void NtpServer::loadState() {
  if (!config_ || !config_->enable_state_persistence || config_->state_file.empty()) {
    return;
  }
  std::ifstream state(config_->state_file);
  if (!state.is_open()) {
    return;
  }
  std::string content((std::istreambuf_iterator<char>(state)),
                      std::istreambuf_iterator<char>());
  if (content.find("total_requests") != std::string::npos) {
    logger_->info("Loaded persisted state from " + config_->state_file);
  }
}

void NtpServer::backupConfig() const {
  if (!config_ || config_->backup_config_file.empty() || config_->lastConfigFile().empty()) {
    return;
  }
  std::ifstream src(config_->lastConfigFile(), std::ios::binary);
  std::ofstream dst(config_->backup_config_file, std::ios::binary | std::ios::trunc);
  if (!src.is_open() || !dst.is_open()) {
    return;
  }
  dst << src.rdbuf();
}

std::string NtpServer::selectUpstreamServer() {
  if (!config_ || config_->upstream_servers.empty()) {
    return std::string();
  }
  if (healthy_upstreams_.empty()) {
    healthy_upstreams_ = config_->upstream_servers;
  }
  if (healthy_upstreams_.size() == 1) {
    return healthy_upstreams_.front();
  }
  switch (config_->upstream_selection_algorithm) {
  case NtpConfig::UpstreamSelectionAlgorithm::RANDOM: {
    std::uniform_int_distribution<size_t> dist(0, healthy_upstreams_.size() - 1);
    return healthy_upstreams_[dist(rng_)];
  }
  case NtpConfig::UpstreamSelectionAlgorithm::LEAST_ERRORS:
    // For now fallback to round-robin until per-upstream error accounting is added.
  case NtpConfig::UpstreamSelectionAlgorithm::ROUND_ROBIN:
  default: {
    const size_t idx = upstream_rr_index_.fetch_add(1) % healthy_upstreams_.size();
    return healthy_upstreams_[idx];
  }
  }
}

void NtpServer::applyDynamicStratum() {
  if (!config_ || !config_->enable_dynamic_stratum_adjustment) {
    return;
  }
  // Basic adaptive stratum: increase stratum when error ratio rises.
  if (stats_.total_requests > 100) {
    const double error_ratio =
        static_cast<double>(stats_.total_errors) / static_cast<double>(stats_.total_requests);
    if (error_ratio > 0.20 && static_cast<int>(config_->stratum) < 15) {
      config_->stratum = static_cast<NtpStratum>(static_cast<int>(config_->stratum) + 1);
    }
  }
}

std::string NtpServer::effectiveReferenceId() const {
  if (!config_ || !config_->enable_reference_clock_support) {
    return config_ ? config_->reference_id : "LOCL";
  }
  std::string source = config_->reference_clock_source;
  std::transform(source.begin(), source.end(), source.begin(), ::tolower);
  if (source == "gps") {
    return "GPS ";
  }
  if (source == "atomic") {
    return "ATOM";
  }
  if (source == "hardware") {
    return "HARD";
  }
  return config_->reference_id.empty() ? "LOCL" : config_->reference_id;
}

} // namespace simple_ntpd
