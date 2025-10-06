/**
 * @file logger.hpp
 * @brief Logging system for NTP daemon
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <fstream>
#include <memory>
#include <mutex>
#include <string>

namespace simple_ntpd {

/**
 * @brief Logging levels
 */
enum class LogLevel { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3, FATAL = 4 };

/**
 * @brief Log destination types
 */
enum class LogDestination {
  CONSOLE = 0,
  FILE = 1,
  SYSLOG = 2,
  BOTH = 3,    // Console and file
  MULTIPLE = 4 // Console, file, and syslog
};

/**
 * @brief Logger class for NTP daemon
 *
 * Provides thread-safe logging with configurable levels and output
 * destinations. Supports console, file, and syslog output.
 */
class Logger {
public:
  /**
   * @brief Default constructor
   */
  Logger();

  /**
   * @brief Destructor
   */
  ~Logger();

  /**
   * @brief Get singleton instance
   * @return Logger instance
   */
  static Logger &getInstance();

  /**
   * @brief Set log level
   * @param level New log level
   */
  void setLevel(LogLevel level);

  /**
   * @brief Set log destination
   * @param destination Log destination
   */
  void setDestination(LogDestination destination);

  /**
   * @brief Set log file
   * @param filename Path to log file
   */
  void setLogFile(const std::string &filename);

  /**
   * @brief Enable/disable syslog
   * @param enable Whether to enable syslog
   * @param facility Syslog facility
   */
  void setSyslog(bool enable, int facility);

  /**
   * @brief Enable or disable structured JSON log output
   * @param enable Whether to enable JSON structured logs
   */
  void setStructuredJson(bool enable);

  /**
   * @brief Log message with custom level
   * @param level Log level
   * @param message Message to log
   * @param file Source file (optional)
   * @param line Source line (optional)
   */
  void log(LogLevel level, const std::string &message,
           const std::string &file = "", int line = 0);

  /**
   * @brief Log debug message
   * @param message Message to log
   * @param file Source file (optional)
   * @param line Source line (optional)
   */
  void debug(const std::string &message, const std::string &file = "",
             int line = 0);

  /**
   * @brief Log info message
   * @param message Message to log
   * @param file Source file (optional)
   * @param line Source line (optional)
   */
  void info(const std::string &message, const std::string &file = "",
            int line = 0);

  /**
   * @brief Log warning message
   * @param message Message to log
   * @param file Source file (optional)
   * @param line Source line (optional)
   */
  void warning(const std::string &message, const std::string &file = "",
               int line = 0);

  /**
   * @brief Log error message
   * @param message Message to log
   * @param file Source file (optional)
   * @param line Source line (optional)
   */
  void error(const std::string &message, const std::string &file = "",
             int line = 0);

  /**
   * @brief Log fatal message
   * @param message Message to log
   * @param file Source file (optional)
   * @param line Source line (optional)
   */
  void fatal(const std::string &message, const std::string &file = "",
             int line = 0);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace simple_ntpd
