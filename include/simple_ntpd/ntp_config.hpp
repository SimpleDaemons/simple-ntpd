/*
 * Copyright 2025 SimpleDaemons
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

#include "simple_ntpd/logger.hpp"
#include "simple_ntpd/platform.hpp"
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
