/**
 * @file ntp_config.hpp
 * @brief NTP server configuration class
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ntpd/utils/logger.hpp"
#include "simple-ntpd/utils/platform.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace simple_ntpd {

/**
 * @brief NTP server configuration
 *
 * Contains all configuration parameters for the NTP daemon
 */
class NtpConfig {
public:
  /**
   * @brief Constructor with default values
   */
  NtpConfig();

  /**
   * @brief Destructor
   */
  ~NtpConfig() = default;

  /**
   * @brief Load configuration from file
   * @param config_file Path to configuration file
   * @return true if successful, false otherwise
   */
  bool loadFromFile(const std::string &config_file);

  /**
   * @brief Load configuration from command line arguments
   * @param argc Argument count
   * @param argv Argument vector
   * @return true if successful, false otherwise
   */
  bool loadFromCommandLine(int argc, char *argv[]);

  /**
   * @brief Validate configuration
   * @return true if valid, false otherwise
   */
  bool validate() const;

  /**
   * @brief Get configuration as string for logging
   * @return Configuration summary string
   */
  std::string toString() const;

  // Network configuration
  std::string listen_address;
  port_t listen_port;
  bool enable_ipv6;
  int max_connections;

  // NTP server configuration
  NtpStratum stratum;
  std::string reference_id;
  std::string reference_clock;
  std::vector<std::string> upstream_servers;
  std::chrono::seconds sync_interval;
  std::chrono::milliseconds timeout;

  // Logging configuration
  std::string log_file;
  LogLevel log_level;
  bool enable_console_logging;
  bool enable_syslog;
  bool log_json;
  // Log rotation
  uint64_t log_max_size_bytes; // 0 disables rotation
  uint32_t log_max_files;      // number of rotated files to keep

  // Security configuration
  bool enable_authentication;
  std::string authentication_key;
  bool restrict_queries;
  std::vector<std::string> allowed_clients;
  std::vector<std::string> denied_clients;

  // Performance configuration
  size_t worker_threads;
  size_t max_packet_size;
  bool enable_statistics;
  std::chrono::seconds stats_interval;

  // Drift file configuration
  std::string drift_file;
  bool enable_drift_compensation;

  // Leap second configuration
  std::string leap_second_file;
  bool enable_leap_second_handling;

private:
  // Path of last-loaded configuration file (if any)
  std::string last_config_file_;

public:
  /**
   * @brief Return last loaded config file path or empty if none
   */
  const std::string &lastConfigFile() const { return last_config_file_; }

  /**
   * @brief Set last loaded config file path
   */
  void setLastConfigFile(const std::string &path) { last_config_file_ = path; }
  /**
   * @brief Set default values
   */
  void setDefaults();

  /**
   * @brief Parse configuration file
   * @param config_file Path to configuration file
   * @return true if successful, false otherwise
   */
  bool parseConfigFile(const std::string &config_file);

  /**
   * @brief Parse command line argument
   * @param arg Argument string
   * @param value Value string
   * @return true if parsed successfully, false otherwise
   */
  bool parseCommandLineArg(const std::string &arg, const std::string &value);
};

} // namespace simple_ntpd
