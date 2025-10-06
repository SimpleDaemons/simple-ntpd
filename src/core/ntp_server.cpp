/**
 * @file ntp_server.cpp
 * @brief NTP server implementation
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple_ntpd/ntp_server.hpp"
#include "simple_ntpd/logger.hpp"
#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/ntp_connection.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <thread>
#ifndef _WIN32
#include <syslog.h>
#endif

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
      cleanup_interval_(std::chrono::seconds(300)) {

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

  // Create or get connection for this client
  auto connection = getOrCreateConnection(client_ip, client_port);
  if (!connection) {
    logger_->warning("Failed to create connection for " +
                     std::string(client_ip) + ":" +
                     std::to_string(client_port));
    return;
  }

  // Process the packet
  if (connection->handlePacket(data)) {
    stats_.total_requests++;
    stats_.total_bytes_transferred += data.size();
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

  return m.str();
}

} // namespace simple_ntpd
