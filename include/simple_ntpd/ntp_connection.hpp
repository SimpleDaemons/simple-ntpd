/*
 * Copyright 2024 BLBurns <contact@blburns.com>
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

#pragma once

#include "simple_ntpd/platform.hpp"
#include "simple_ntpd/ntp_packet.hpp"
#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/logger.hpp"
#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <functional>

namespace simple_ntpd {

/**
 * @brief NTP connection statistics
 */
struct NtpConnectionStats {
    uint64_t packets_received;
    uint64_t packets_sent;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    uint64_t errors;
    std::chrono::steady_clock::time_point connection_time;
    std::chrono::steady_clock::time_point last_activity;
    
    NtpConnectionStats() : packets_received(0), packets_sent(0),
                          bytes_received(0), bytes_sent(0), errors(0) {}
};

/**
 * @brief NTP connection class
 * 
 * Manages a single client connection to the NTP server
 */
class NtpConnection {
public:
    /**
     * @brief Constructor
     * @param socket Client socket
     * @param client_address Client address
     * @param config Server configuration
     * @param logger Logger instance
     */
    NtpConnection(socket_t socket,
                 const std::string& client_address,
                 std::shared_ptr<NtpConfig> config,
                 std::shared_ptr<Logger> logger);
    
    /**
     * @brief Destructor
     */
    ~NtpConnection();
    
    /**
     * @brief Start connection handling
     */
    void start();
    
    /**
     * @brief Stop connection handling
     */
    void stop();
    
    /**
     * @brief Check if connection is active
     * @return true if active, false otherwise
     */
    bool isActive() const;
    
    /**
     * @brief Get client address
     * @return Client address string
     */
    std::string getClientAddress() const;
    
    /**
     * @brief Get connection statistics
     * @return Connection statistics
     */
    NtpConnectionStats getStats() const;
    
    /**
     * @brief Get connection status
     * @return Status string
     */
    std::string getStatus() const;
    
    /**
     * @brief Set connection callback
     * @param callback Callback function
     */
    void setConnectionCallback(std::function<void(const std::string&, bool)> callback);

private:
    /**
     * @brief Main connection loop
     */
    void connectionLoop();
    
    /**
     * @brief Handle incoming packet
     * @param data Raw packet data
     * @return true if handled successfully, false otherwise
     */
    bool handlePacket(const std::vector<uint8_t>& data);
    
    /**
     * @brief Send response packet
     * @param packet Response packet
     * @return true if sent successfully, false otherwise
     */
    bool sendResponse(const NtpPacket& packet);
    
    /**
     * @brief Read data from socket
     * @param buffer Buffer to read into
     * @param max_size Maximum bytes to read
     * @return Number of bytes read, -1 on error
     */
    ssize_t readFromSocket(std::vector<uint8_t>& buffer, size_t max_size);
    
    /**
     * @brief Write data to socket
     * @param data Data to write
     * @return true if successful, false otherwise
     */
    bool writeToSocket(const std::vector<uint8_t>& data);
    
    /**
     * @brief Update connection statistics
     * @param received_bytes Bytes received
     * @param sent_bytes Bytes sent
     */
    void updateStats(size_t received_bytes, size_t sent_bytes);
    
    /**
     * @brief Log connection activity
     * @param message Log message
     * @param level Log level
     */
    void logActivity(const std::string& message, LogLevel level = LogLevel::INFO);
    
    /**
     * @brief Handle connection error
     * @param error_message Error message
     */
    void handleError(const std::string& error_message);

private:
    socket_t client_socket_;
    std::string client_address_;
    std::shared_ptr<NtpConfig> config_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<NtpPacketHandler> packet_handler_;
    
    std::atomic<bool> active_;
    std::atomic<bool> shutdown_requested_;
    std::thread connection_thread_;
    
    NtpConnectionStats stats_;
    std::function<void(const std::string&, bool)> connection_callback_;
    
    // Connection state
    std::chrono::steady_clock::time_point last_packet_time_;
    NtpPacket last_request_packet_;
    bool authenticated_;
    
    // Buffer for packet processing
    std::vector<uint8_t> receive_buffer_;
    static constexpr size_t MAX_PACKET_SIZE = 1024;
};

} // namespace simple_ntpd
