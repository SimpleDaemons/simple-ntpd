#include "simple_ntpd/ntp_server.hpp"
#include "simple_ntpd/logger.hpp"
#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/ntp_connection.hpp"
#include <algorithm>
#include <cstring>
#include <thread>

namespace simple_ntpd {

// NtpServer implementation
NtpServer::NtpServer(const NtpConfig& config)
    : config_(config),
      is_running_(false),
      server_socket_(INVALID_SOCKET),
      connection_manager_(config.performance.max_connections),
      worker_threads_(),
      statistics_() {
    
    Logger::getInstance().info("NTP Server initialized with configuration");
    Logger::getInstance().debug("Server will listen on " + config.network.listen_address + 
                              ":" + std::to_string(config.network.listen_port));
}

NtpServer::~NtpServer() {
    stop();
}

bool NtpServer::start() {
    if (is_running_) {
        Logger::getInstance().warning("NTP Server is already running");
        return false;
    }
    
    Logger::getInstance().info("Starting NTP Server...");
    
    // Initialize socket
    if (!initializeSocket()) {
        Logger::getInstance().error("Failed to initialize server socket");
        return false;
    }
    
    // Bind socket
    if (!bindSocket()) {
        Logger::getInstance().error("Failed to bind server socket");
        cleanupSocket();
        return false;
    }
    
    // Start listening
    if (!startListening()) {
        Logger::getInstance().error("Failed to start listening on socket");
        cleanupSocket();
        return false;
    }
    
    // Start worker threads
    if (!startWorkerThreads()) {
        Logger::getInstance().error("Failed to start worker threads");
        cleanupSocket();
        return false;
    }
    
    is_running_ = true;
    start_time_ = std::chrono::system_clock::now();
    
    Logger::getInstance().info("NTP Server started successfully");
    Logger::getInstance().info("Listening on " + config_.network.listen_address + 
                              ":" + std::to_string(config_.network.listen_port));
    
    return true;
}

void NtpServer::stop() {
    if (!is_running_) {
        return;
    }
    
    Logger::getInstance().info("Stopping NTP Server...");
    
    is_running_ = false;
    
    // Stop worker threads
    stopWorkerThreads();
    
    // Close all connections
    connection_manager_.closeAllConnections();
    
    // Cleanup socket
    cleanupSocket();
    
    Logger::getInstance().info("NTP Server stopped");
}

bool NtpServer::isRunning() const {
    return is_running_;
}

bool NtpServer::initializeSocket() {
    #ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        Logger::getInstance().error("Failed to initialize Winsock");
        return false;
    }
    #endif
    
    server_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_socket_ == INVALID_SOCKET) {
        Logger::getInstance().error("Failed to create socket: " + std::strerror(errno));
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        Logger::getInstance().warning("Failed to set SO_REUSEADDR: " + std::strerror(errno));
    }
    
    // Set non-blocking mode
    #ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(server_socket_, FIONBIO, &mode) != 0) {
        Logger::getInstance().warning("Failed to set non-blocking mode");
    }
    #else
    int flags = fcntl(server_socket_, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
    }
    #endif
    
    return true;
}

bool NtpServer::bindSocket() {
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config_.network.listen_port);
    
    if (inet_pton(AF_INET, config_.network.listen_address.c_str(), &server_addr.sin_addr) <= 0) {
        Logger::getInstance().error("Invalid listen address: " + config_.network.listen_address);
        return false;
    }
    
    if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&server_addr), 
             sizeof(server_addr)) < 0) {
        Logger::getInstance().error("Failed to bind socket: " + std::strerror(errno));
        return false;
    }
    
    return true;
}

bool NtpServer::startListening() {
    // For UDP, we don't need to call listen()
    // The socket is ready to receive datagrams after binding
    return true;
}

bool NtpServer::startWorkerThreads() {
    size_t thread_count = config_.performance.worker_threads;
    
    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&NtpServer::workerThreadFunction, this, i);
        Logger::getInstance().debug("Started worker thread " + std::to_string(i));
    }
    
    return true;
}

void NtpServer::stopWorkerThreads() {
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    Logger::getInstance().info("All worker threads stopped");
}

void NtpServer::workerThreadFunction(size_t thread_id) {
    Logger::getInstance().debug("Worker thread " + std::to_string(thread_id) + " started");
    
    while (is_running_) {
        // Process incoming packets
        processIncomingPackets();
        
        // Clean up inactive connections
        connection_manager_.closeInactiveConnections(
            std::chrono::seconds(config_.performance.connection_timeout)
        );
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    Logger::getInstance().debug("Worker thread " + std::to_string(thread_id) + " stopped");
}

void NtpServer::processIncomingPackets() {
    std::vector<uint8_t> buffer(NTP_PACKET_SIZE);
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    while (is_running_) {
        std::memset(&client_addr, 0, sizeof(client_addr));
        
        ssize_t bytes_received = recvfrom(server_socket_, buffer.data(), buffer.size(), 0,
                                        reinterpret_cast<struct sockaddr*>(&client_addr), 
                                        &client_addr_len);
        
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available (non-blocking socket)
                break;
            }
            
            Logger::getInstance().error("Failed to receive data: " + std::strerror(errno));
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
        Logger::getInstance().warning("Failed to create connection for " + 
                                    std::string(client_ip) + ":" + std::to_string(client_port));
        return;
    }
    
    // Process the packet
    if (connection->processPacket(data)) {
        statistics_.packets_processed++;
        statistics_.bytes_processed += data.size();
    } else {
        statistics_.packets_errors++;
    }
}

std::shared_ptr<NtpConnection> NtpServer::getOrCreateConnection(const std::string& client_ip, 
                                                               uint16_t client_port) {
    // For UDP, we create a new connection object for each client
    // In a more sophisticated implementation, we might want to maintain
    // a connection cache based on client address
    
    // Create a dummy socket for the connection object
    // In practice, we'd handle this differently for UDP
    socket_t dummy_socket = INVALID_SOCKET;
    
    auto connection = connection_manager_.createConnection(dummy_socket, client_ip, client_port);
    if (connection) {
        // Process the packet directly since we don't have a real socket
        // This is a simplified approach for UDP
        return connection;
    }
    
    return nullptr;
}

void NtpServer::cleanupSocket() {
    if (server_socket_ != INVALID_SOCKET) {
        #ifdef _WIN32
        closesocket(server_socket_);
        WSACleanup();
        #else
        ::close(server_socket_);
        #endif
        server_socket_ = INVALID_SOCKET;
    }
}

const NtpConfig& NtpServer::getConfig() const {
    return config_;
}

const NtpConnectionManager& NtpServer::getConnectionManager() const {
    return connection_manager_;
}

NtpServerStatistics NtpServer::getStatistics() const {
    std::lock_guard<std::mutex> lock(statistics_mutex_);
    return statistics_;
}

std::string NtpServer::getServerStatus() const {
    std::stringstream ss;
    
    ss << "NTP Server Status:\n";
    ss << "  Status: " << (is_running_ ? "Running" : "Stopped") << "\n";
    
    if (is_running_) {
        auto uptime = std::chrono::system_clock::now() - start_time_;
        auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
        ss << "  Uptime: " << uptime_seconds << " seconds\n";
        ss << "  Listen Address: " << config_.network.listen_address << ":" 
           << config_.network.listen_port << "\n";
        ss << "  Worker Threads: " << worker_threads_.size() << "\n";
        ss << "  Active Connections: " << connection_manager_.getActiveConnectionCount() << "\n";
        
        auto stats = getStatistics();
        ss << "  Packets Processed: " << stats.packets_processed << "\n";
        ss << "  Bytes Processed: " << stats.bytes_processed << "\n";
        ss << "  Errors: " << stats.packets_errors << "\n";
    }
    
    return ss.str();
}

void NtpServer::updateStatistics(const NtpPacket& packet, bool success) {
    std::lock_guard<std::mutex> lock(statistics_mutex_);
    
    if (success) {
        statistics_.packets_processed++;
        statistics_.bytes_processed += NTP_PACKET_SIZE;
    } else {
        statistics_.packets_errors++;
    }
}

} // namespace simple_ntpd
