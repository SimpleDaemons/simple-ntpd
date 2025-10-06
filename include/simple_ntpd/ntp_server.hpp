/**
 * @file ntp_server.hpp
 * @brief NTP server class definition
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple_ntpd/logger.hpp"
#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/ntp_connection.hpp"
#include "simple_ntpd/platform.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <vector>

namespace simple_ntpd {

/**
 * @brief NTP server statistics
 */
struct NtpServerStats {
  uint64_t total_connections;
  uint64_t active_connections;
  uint64_t total_requests;
  uint64_t total_responses;
  uint64_t total_bytes_transferred;
  uint64_t total_errors;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point last_activity;

  NtpServerStats()
      : total_connections(0), active_connections(0), total_requests(0),
        total_responses(0), total_bytes_transferred(0), total_errors(0) {}
};

/**
 * @brief NTP server class
 *
 * Main server class that manages the NTP daemon including:
 * - Listening for incoming connections
 * - Managing multiple NTP connections
 * - Configuration management
 * - Statistics collection
 * - Graceful shutdown
 */
class NtpServer {
public:
  /**
   * @brief Constructor
   * @param config Server configuration
   * @param logger Logger instance
   */
  NtpServer(std::shared_ptr<NtpConfig> config, std::shared_ptr<Logger> logger);

  /**
   * @brief Destructor
   */
  ~NtpServer();

  /**
   * @brief Start the NTP server
   * @return true if started successfully, false otherwise
   */
  bool start();

  /**
   * @brief Stop the NTP server
   */
  void stop();

  /**
   * @brief Check if server is running
   * @return true if running, false otherwise
   */
  bool isRunning() const;

  /**
   * @brief Get server status information
   * @return Status string
   */
  std::string getStatus() const;

  /**
   * @brief Get active connection count
   * @return Number of active connections
   */
  size_t getActiveConnectionCount() const;

  /**
   * @brief Get server statistics
   * @return Server statistics
   */
  NtpServerStats getStats() const;

  /**
   * @brief Reload configuration
   * @return true if successful, false otherwise
   */
  bool reloadConfig();

  /**
   * @brief Get server configuration
   * @return Server configuration
   */
  std::shared_ptr<NtpConfig> getConfig() const;

  /**
   * @brief Set configuration change callback
   * @param callback Callback function
   */
  void setConfigChangeCallback(std::function<void()> callback);

private:
  /**
   * @brief Main server loop
   */
  void serverLoop();

  /**
   * @brief Accept new connections
   */
  void acceptConnections();

  /**
   * @brief Handle new connection
   * @param client_socket Client socket
   * @param client_address Client address
   */
  void handleNewConnection(socket_t client_socket,
                           const std::string &client_address);

  /**
   * @brief Connection callback
   * @param client_address Client address
   * @param connected Whether connected or disconnected
   */
  void onConnectionChange(const std::string &client_address, bool connected);

  /**
   * @brief Clean up inactive connections
   */
  void cleanupConnections();

  /**
   * @brief Initialize server socket
   * @return true if successful, false otherwise
   */
  bool initializeSocket();

  /**
   * @brief Bind server socket
   * @return true if successful, false otherwise
   */
  bool bindSocket();

  /**
   * @brief Close server socket
   */
  void closeSocket();

  /**
   * @brief Update server statistics
   * @param connection_stats Connection statistics
   */
  void updateStats(const NtpConnectionStats &connection_stats);

  /**
   * @brief Log server activity
   * @param message Log message
   * @param level Log level
   */
  void logActivity(const std::string &message, LogLevel level = LogLevel::INFO);

  /**
   * @brief Handle server error
   * @param error_message Error message
   */
  void handleError(const std::string &error_message);

  /**
   * @brief Start worker threads
   */
  void startWorkerThreads();

  /**
   * @brief Stop worker threads
   */
  void stopWorkerThreads();

  /**
   * @brief Worker thread function
   * @param thread_id Worker thread ID
   */
  void workerThreadFunction(size_t thread_id);

  /**
   * @brief Process incoming packets
   */
  void processIncomingPackets();

  /**
   * @brief Process a single packet
   * @param data Packet data
   * @param client_addr Client address
   */
  void processPacket(const std::vector<uint8_t> &data,
                     const struct sockaddr_in &client_addr);

  /**
   * @brief Get or create connection for client
   * @param client_ip Client IP address
   * @param client_port Client port
   * @return Connection object
   */
  std::shared_ptr<NtpConnection>
  getOrCreateConnection(const std::string &client_ip, uint16_t client_port);

private:
  std::shared_ptr<NtpConfig> config_;
  std::shared_ptr<Logger> logger_;

  // Server state
  std::atomic<bool> running_;
  std::atomic<bool> shutdown_requested_;

  // Socket management
  socket_t server_socket_;
  std::string server_address_;
  port_t server_port_;

  // Connection management
  std::map<std::string, std::shared_ptr<NtpConnection>> active_connections_;
  mutable std::mutex connections_mutex_;

  // Threading
  std::thread accept_thread_;
  std::vector<std::thread> worker_threads_;
  std::atomic<bool> workers_running_;

  // Config watching
  std::thread config_watch_thread_;
  std::atomic<bool> config_watch_running_;
  std::chrono::system_clock::time_point config_last_write_time_;
  void startConfigWatcher();
  void stopConfigWatcher();
  void configWatcherLoop();

  // Statistics
  NtpServerStats stats_;
  mutable std::mutex stats_mutex_;

  // Configuration change callback
  std::function<void()> config_change_callback_;

  // Server timing
  std::chrono::steady_clock::time_point last_cleanup_time_;
  std::chrono::seconds cleanup_interval_;

  // Platform-specific data
  struct sockaddr_in server_addr_;
#ifdef ENABLE_IPV6
  struct sockaddr_in6 server_addr6_;
#endif
};

} // namespace simple_ntpd
