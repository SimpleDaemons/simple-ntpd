#include "simple_ntpd/ntp_server.hpp"
#include "simple_ntpd/logger.hpp"
#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/ntp_connection.hpp"
#include <algorithm>
#include <cstring>
#include <thread>
#include <sstream>

namespace simple_ntpd {

// NtpServer implementation
NtpServer::NtpServer(std::shared_ptr<NtpConfig> config, std::shared_ptr<Logger> logger)
    : config_(config),
      logger_(logger),
      running_(false),
      shutdown_requested_(false),
      server_socket_(INVALID_SOCKET),
      server_address_(config->listen_address),
      server_port_(config->listen_port),
      active_connections_(),
      connections_mutex_(),
      accept_thread_(),
      worker_threads_(),
      workers_running_(false),
      stats_(),
      stats_mutex_(),
      config_change_callback_(),
      last_cleanup_time_(std::chrono::steady_clock::now()),
      cleanup_interval_(std::chrono::seconds(300)) {
    
    logger_->info("NTP Server initialized with configuration");
    logger_->debug("Server will listen on " + config->listen_address + 
                  ":" + std::to_string(config->listen_port));
}

NtpServer::~NtpServer() {
    stop();
}

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
    
    running_ = true;
    stats_.start_time = std::chrono::steady_clock::now();
    
    logger_->info("NTP Server started successfully");
    logger_->info("Listening on " + config_->listen_address + 
                  ":" + std::to_string(config_->listen_port));
    
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
    
    // Close all connections
    cleanupConnections();
    
    // Cleanup socket
    closeSocket();
    
    logger_->info("NTP Server stopped");
}

bool NtpServer::isRunning() const {
    return running_;
}

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
        logger_->error("Failed to create socket: " + std::string(std::strerror(errno)));
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        logger_->warning("Failed to set SO_REUSEADDR: " + std::string(std::strerror(errno)));
    }
    
    return true;
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
    
    if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&server_addr), 
             sizeof(server_addr)) < 0) {
        logger_->error("Failed to bind socket: " + std::string(std::strerror(errno)));
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
    
    for (auto& thread : worker_threads_) {
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
        
        ssize_t bytes_received = recvfrom(server_socket_, buffer.data(), buffer.size(), 0,
                                        reinterpret_cast<struct sockaddr*>(&client_addr), 
                                        &client_addr_len);
        
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available (non-blocking socket)
                break;
            }
            
            logger_->error("Failed to receive data: " + std::string(std::strerror(errno)));
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

void NtpServer::processPacket(const std::vector<uint8_t>& data, 
                             const struct sockaddr_in& client_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr.sin_port);
    
    // Create or get connection for this client
    auto connection = getOrCreateConnection(client_ip, client_port);
    if (!connection) {
        logger_->warning("Failed to create connection for " + 
                       std::string(client_ip) + ":" + std::to_string(client_port));
        return;
    }
    
    // Process the packet
    if (connection->handlePacket(data)) {
        stats_.total_requests++;
        stats_.total_bytes_transferred += data.size();
    } else {
        stats_.total_errors++;
    }
}

std::shared_ptr<NtpConnection> NtpServer::getOrCreateConnection(const std::string& client_ip, 
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
    
    auto connection = std::make_shared<NtpConnection>(dummy_socket, client_ip, config_, logger_);
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

std::shared_ptr<NtpConfig> NtpServer::getConfig() const {
    return config_;
}

std::string NtpServer::getStatus() const {
    std::stringstream ss;
    
    ss << "NTP Server Status:\n";
    ss << "  Status: " << (running_ ? "Running" : "Stopped") << "\n";
    
    if (running_) {
        auto uptime = std::chrono::steady_clock::now() - stats_.start_time;
        auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
        ss << "  Uptime: " << uptime_seconds << " seconds\n";
        ss << "  Listen Address: " << server_address_ << ":" << server_port_ << "\n";
        ss << "  Worker Threads: " << worker_threads_.size() << "\n";
        ss << "  Active Connections: " << stats_.active_connections << "\n";
        ss << "  Total Requests: " << stats_.total_requests << "\n";
        ss << "  Total Bytes: " << stats_.total_bytes_transferred << "\n";
        ss << "  Total Errors: " << stats_.total_errors << "\n";
    }
    
    return ss.str();
}

} // namespace simple_ntpd
