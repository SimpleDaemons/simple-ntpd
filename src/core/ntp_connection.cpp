#include "simple_ntpd/ntp_connection.hpp"
#include "simple_ntpd/logger.hpp"
#include "simple_ntpd/ntp_packet.hpp"
#include <chrono>
#include <cstring>

namespace simple_ntpd {

// NtpConnection implementation
NtpConnection::NtpConnection(socket_t socket, const std::string& client_address, uint16_t client_port)
    : socket_(socket), 
      client_address_(client_address), 
      client_port_(client_port),
      is_active_(true),
      created_time_(std::chrono::system_clock::now()),
      last_activity_(std::chrono::system_clock::now()),
      packets_received_(0),
      packets_sent_(0),
      bytes_received_(0),
      bytes_sent_(0),
      errors_(0) {
    
    Logger::getInstance().debug("New NTP connection from " + client_address_ + ":" + std::to_string(client_port_));
}

NtpConnection::~NtpConnection() {
    close();
}

bool NtpConnection::receivePacket(std::vector<uint8_t>& data) {
    if (!is_active_) {
        return false;
    }
    
    data.resize(NTP_PACKET_SIZE);
    ssize_t bytes_received = recv(socket_, data.data(), data.size(), 0);
    
    if (bytes_received < 0) {
        Logger::getInstance().error("Failed to receive data from " + client_address_ + ": " + 
                                  std::strerror(errno));
        errors_++;
        return false;
    }
    
    if (bytes_received == 0) {
        // Connection closed by client
        Logger::getInstance().debug("Connection closed by " + client_address_);
        is_active_ = false;
        return false;
    }
    
    data.resize(bytes_received);
    bytes_received_ += bytes_received;
    packets_received_++;
    last_activity_ = std::chrono::system_clock::now();
    
    Logger::getInstance().debug("Received " + std::to_string(bytes_received) + " bytes from " + 
                              client_address_);
    
    return true;
}

bool NtpConnection::sendPacket(const std::vector<uint8_t>& data) {
    if (!is_active_) {
        return false;
    }
    
    ssize_t bytes_sent = send(socket_, data.data(), data.size(), 0);
    
    if (bytes_sent < 0) {
        Logger::getInstance().error("Failed to send data to " + client_address_ + ": " + 
                                  std::strerror(errno));
        errors_++;
        return false;
    }
    
    bytes_sent_ += bytes_sent;
    packets_sent_++;
    last_activity_ = std::chrono::system_clock::now();
    
    Logger::getInstance().debug("Sent " + std::to_string(bytes_sent) + " bytes to " + 
                              client_address_);
    
    return true;
}

bool NtpConnection::processPacket(const std::vector<uint8_t>& data) {
    if (data.size() < NTP_PACKET_SIZE) {
        Logger::getInstance().warning("Received packet too short from " + client_address_ + 
                                    ": " + std::to_string(data.size()) + " bytes");
        return false;
    }
    
    // Parse the NTP packet
    NtpPacket packet;
    if (!packet.parse(data.data(), data.size())) {
        Logger::getInstance().warning("Failed to parse NTP packet from " + client_address_);
        return false;
    }
    
    // Validate the packet
    if (!packet.isValid()) {
        Logger::getInstance().warning("Invalid NTP packet from " + client_address_);
        return false;
    }
    
    // Check if this is a client request
    if (packet.getMode() != NtpMode::CLIENT) {
        Logger::getInstance().warning("Received non-client packet from " + client_address_ + 
                                    " (mode: " + std::to_string(static_cast<int>(packet.getMode())) + ")");
        return false;
    }
    
    // Create server response
    auto response = createServerResponse(packet);
    if (!response) {
        Logger::getInstance().error("Failed to create server response for " + client_address_);
        return false;
    }
    
    // Send the response
    auto response_data = response->serialize();
    if (!sendPacket(response_data)) {
        Logger::getInstance().error("Failed to send response to " + client_address_);
        return false;
    }
    
    Logger::getInstance().debug("Processed NTP request from " + client_address_ + 
                              " (stratum: " + std::to_string(response->getStratum()) + ")");
    
    return true;
}

std::unique_ptr<NtpPacket> NtpConnection::createServerResponse(const NtpPacket& request) {
    // Get current time for reference timestamp
    auto now = std::chrono::system_clock::now();
    NtpTimestamp reference_ts(now);
    
    // Create response packet
    auto response = NtpPacketHandler::createServerResponse(
        request,
        getServerStratum(),
        reference_ts,
        getReferenceId()
    );
    
    return response;
}

uint8_t NtpConnection::getServerStratum() const {
    // This would typically come from configuration or upstream servers
    // For now, return a default stratum
    return 2;
}

uint32_t NtpConnection::getReferenceId() const {
    // This would typically be the reference clock identifier
    // For now, return a default value
    return 0x4C4F434C; // "LOCL" in ASCII
}

void NtpConnection::close() {
    if (is_active_) {
        is_active_ = false;
        
        if (socket_ != INVALID_SOCKET) {
            #ifdef _WIN32
            closesocket(socket_);
            #else
            ::close(socket_);
            #endif
            socket_ = INVALID_SOCKET;
        }
        
        Logger::getInstance().debug("Closed NTP connection to " + client_address_ + 
                                  " (packets: " + std::to_string(packets_received_) + 
                                  " received, " + std::to_string(packets_sent_) + " sent)");
    }
}

bool NtpConnection::isActive() const {
    return is_active_;
}

bool NtpConnection::isIdle(const std::chrono::seconds& timeout) const {
    auto now = std::chrono::system_clock::now();
    auto idle_time = now - last_activity_;
    return idle_time > timeout;
}

socket_t NtpConnection::getSocket() const {
    return socket_;
}

const std::string& NtpConnection::getClientAddress() const {
    return client_address_;
}

uint16_t NtpConnection::getClientPort() const {
    return client_port_;
}

std::chrono::system_clock::time_point NtpConnection::getCreatedTime() const {
    return created_time_;
}

std::chrono::system_clock::time_point NtpConnection::getLastActivity() const {
    return last_activity_;
}

uint64_t NtpConnection::getPacketsReceived() const {
    return packets_received_;
}

uint64_t NtpConnection::getPacketsSent() const {
    return packets_sent_;
}

uint64_t NtpConnection::getBytesReceived() const {
    return bytes_received_;
}

uint64_t NtpConnection::getBytesSent() const {
    return bytes_sent_;
}

uint64_t NtpConnection::getErrors() const {
    return errors_;
}

double NtpConnection::getUptime() const {
    auto now = std::chrono::system_clock::now();
    auto uptime = now - created_time_;
    return std::chrono::duration<double>(uptime).count();
}

std::string NtpConnection::getStatistics() const {
    std::stringstream ss;
    ss << "Connection Statistics for " << client_address_ << ":" << client_port_ << "\n";
    ss << "  Status: " << (is_active_ ? "Active" : "Closed") << "\n";
    ss << "  Uptime: " << std::fixed << std::setprecision(2) << getUptime() << " seconds\n";
    ss << "  Packets Received: " << packets_received_ << "\n";
    ss << "  Packets Sent: " << packets_sent_ << "\n";
    ss << "  Bytes Received: " << bytes_received_ << "\n";
    ss << "  Bytes Sent: " << bytes_sent_ << "\n";
    ss << "  Errors: " << errors_ << "\n";
    ss << "  Last Activity: " << std::chrono::duration_cast<std::chrono::seconds>(
        last_activity_.time_since_epoch()).count() << " seconds ago";
    
    return ss.str();
}

// NtpConnectionManager implementation
NtpConnectionManager::NtpConnectionManager(size_t max_connections)
    : max_connections_(max_connections) {
    
    Logger::getInstance().info("NTP Connection Manager initialized with max " + 
                              std::to_string(max_connections_) + " connections");
}

NtpConnectionManager::~NtpConnectionManager() {
    closeAllConnections();
}

std::shared_ptr<NtpConnection> NtpConnectionManager::createConnection(socket_t socket, 
                                                                     const std::string& client_address, 
                                                                     uint16_t client_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (connections_.size() >= max_connections_) {
        Logger::getInstance().warning("Maximum connections reached, rejecting connection from " + 
                                    client_address);
        return nullptr;
    }
    
    auto connection = std::make_shared<NtpConnection>(socket, client_address, client_port);
    connections_.push_back(connection);
    
    Logger::getInstance().debug("Created connection " + std::to_string(connections_.size()) + 
                              "/" + std::to_string(max_connections_));
    
    return connection;
}

void NtpConnectionManager::removeConnection(const std::shared_ptr<NtpConnection>& connection) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find(connections_.begin(), connections_.end(), connection);
    if (it != connections_.end()) {
        connections_.erase(it);
        Logger::getInstance().debug("Removed connection, total: " + 
                                  std::to_string(connections_.size()));
    }
}

void NtpConnectionManager::closeInactiveConnections(const std::chrono::seconds& timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = connections_.begin();
    while (it != connections_.end()) {
        if ((*it)->isIdle(timeout)) {
            Logger::getInstance().debug("Closing inactive connection to " + 
                                      (*it)->getClientAddress());
            (*it)->close();
            it = connections_.erase(it);
        } else {
            ++it;
        }
    }
}

void NtpConnectionManager::closeAllConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& connection : connections_) {
        connection->close();
    }
    
    connections_.clear();
    Logger::getInstance().info("Closed all NTP connections");
}

size_t NtpConnectionManager::getActiveConnectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

size_t NtpConnectionManager::getMaxConnections() const {
    return max_connections_;
}

std::vector<std::shared_ptr<NtpConnection>> NtpConnectionManager::getConnections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_;
}

std::string NtpConnectionManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "Connection Manager Statistics:\n";
    ss << "  Active Connections: " << connections_.size() << "/" << max_connections_ << "\n";
    ss << "  Available Slots: " << (max_connections_ - connections_.size()) << "\n";
    
    if (!connections_.empty()) {
        ss << "\nConnection Details:\n";
        for (const auto& conn : connections_) {
            ss << "  " << conn->getClientAddress() << ":" << conn->getClientPort() 
               << " - " << (conn->isActive() ? "Active" : "Closed") << "\n";
        }
    }
    
    return ss.str();
}

} // namespace simple_ntpd
