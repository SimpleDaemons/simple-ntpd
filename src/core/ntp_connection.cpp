/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "simple_ntpd/ntp_connection.hpp"
#include "simple_ntpd/logger.hpp"
#include "simple_ntpd/ntp_packet.hpp"
#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

namespace simple_ntpd {

// NtpConnection implementation
NtpConnection::NtpConnection(socket_t socket,
                           const std::string& client_address,
                           std::shared_ptr<NtpConfig> config,
                           std::shared_ptr<Logger> logger)
    : client_socket_(socket),
      client_address_(client_address),
      config_(config),
      logger_(logger),
      active_(true),
      stats_() {
    
    logger_->debug("New NTP connection from " + client_address_);
}

NtpConnection::~NtpConnection() {
    stop();
}

void NtpConnection::start() {
    if (active_) {
        logger_->debug("Connection already active for " + client_address_);
        return;
    }
    
    active_ = true;
    logger_->debug("Started connection for " + client_address_);
}

void NtpConnection::stop() {
    if (!active_) {
        return;
    }
    
    active_ = false;
    
    if (client_socket_ != INVALID_SOCKET) {
        #ifdef _WIN32
        closesocket(client_socket_);
        #else
        CLOSE_SOCKET(client_socket_);
        #endif
        client_socket_ = INVALID_SOCKET;
    }
    
    logger_->debug("Stopped connection for " + client_address_);
}

bool NtpConnection::isActive() const {
    return active_;
}

std::string NtpConnection::getClientAddress() const {
    return client_address_;
}

NtpConnectionStats NtpConnection::getStats() const {
    return stats_;
}

std::string NtpConnection::getStatus() const {
    std::stringstream ss;
    ss << "Connection Status for " << client_address_ << ":\n";
    ss << "  Active: " << (active_ ? "Yes" : "No") << "\n";
    ss << "  Socket: " << client_socket_ << "\n";
    return ss.str();
}

void NtpConnection::setConnectionCallback(std::function<void(const std::string&, bool)> callback) {
    connection_callback_ = callback;
}

bool NtpConnection::handlePacket(const std::vector<uint8_t>& data) {
    if (!active_) {
        return false;
    }
    
    if (data.size() < NTP_PACKET_SIZE) {
        logger_->warning("Received packet too short from " + client_address_ + 
                        ": " + std::to_string(data.size()) + " bytes");
        return false;
    }
    
    // Parse the NTP packet
    NtpPacket packet;
    if (!packet.parseFromData(data)) {
        logger_->warning("Failed to parse NTP packet from " + client_address_);
        return false;
    }
    
    // Validate the packet
    if (!packet.isValid()) {
        logger_->warning("Invalid NTP packet from " + client_address_);
        return false;
    }
    
    // Check if this is a client request
    if (packet.mode != NtpMode::CLIENT) {
        logger_->warning("Received non-client packet from " + client_address_ + 
                        " (mode: " + std::to_string(static_cast<int>(packet.mode)) + ")");
        return false;
    }
    
    // Create server response
    auto response = NtpPacket::createServerResponse(packet, NtpStratum::SECONDARY_REFERENCE, "LOCL");
    
    // Send the response
    auto response_data = response.serializeToData();
    if (!sendResponse(response)) {
        logger_->error("Failed to send response to " + client_address_);
        return false;
    }
    
    logger_->debug("Processed NTP request from " + client_address_ + 
                  " (stratum: " + std::to_string(static_cast<int>(response.stratum)) + ")");
    
    // Update statistics
    stats_.packets_received++;
    stats_.bytes_received += data.size();
    
    return true;
}



bool NtpConnection::sendResponse(const NtpPacket& packet) {
    auto data = packet.serializeToData();
    
    if (writeToSocket(data)) {
        stats_.packets_sent++;
        stats_.bytes_sent += data.size();
        return true;
    }
    
    return false;
}

ssize_t NtpConnection::readFromSocket(std::vector<uint8_t>& buffer, size_t max_size) {
    if (!active_ || client_socket_ == INVALID_SOCKET) {
        return -1;
    }
    
    buffer.resize(max_size);
    ssize_t bytes_read = recv(client_socket_, buffer.data(), buffer.size(), 0);
    
    if (bytes_read < 0) {
        logger_->error("Failed to read from socket: " + std::string(std::strerror(errno)));
        return -1;
    }
    
    buffer.resize(bytes_read);
    return bytes_read;
}

bool NtpConnection::writeToSocket(const std::vector<uint8_t>& data) {
    if (!active_ || client_socket_ == INVALID_SOCKET) {
        return false;
    }
    
    ssize_t bytes_sent = send(client_socket_, data.data(), data.size(), 0);
    
    if (bytes_sent < 0) {
        logger_->error("Failed to write to socket: " + std::string(std::strerror(errno)));
        return false;
    }
    
    return bytes_sent == static_cast<ssize_t>(data.size());
}

void NtpConnection::updateStats(size_t received_bytes, size_t sent_bytes) {
    stats_.bytes_received += received_bytes;
    stats_.bytes_sent += sent_bytes;
}

void NtpConnection::logActivity(const std::string& message, LogLevel level) {
    if (logger_) {
        switch (level) {
            case LogLevel::DEBUG:
                logger_->debug(message);
                break;
            case LogLevel::INFO:
                logger_->info(message);
                break;
            case LogLevel::WARNING:
                logger_->warning(message);
                break;
            case LogLevel::ERROR:
                logger_->error(message);
                break;
            case LogLevel::FATAL:
                logger_->fatal(message);
                break;
        }
    }
}

void NtpConnection::handleError(const std::string& error_message) {
    logger_->error("Connection error for " + client_address_ + ": " + error_message);
    stats_.errors++;
}

void NtpConnection::connectionLoop() {
    logger_->debug("Starting connection loop for " + client_address_);
    
    while (active_) {
        std::vector<uint8_t> buffer;
        ssize_t bytes_read = readFromSocket(buffer, NTP_PACKET_SIZE);
        
        if (bytes_read < 0) {
            // Error or connection closed
            break;
        }
        
        if (bytes_read == 0) {
            // Connection closed by client
            break;
        }
        
        // Handle the received packet
        if (!handlePacket(buffer)) {
            stats_.errors++;
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    logger_->debug("Connection loop ended for " + client_address_);
}

} // namespace simple_ntpd
