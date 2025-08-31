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

#include <string>
#include <memory>
#include <fstream>
#include <mutex>

namespace simple_ntpd {

/**
 * @brief Logging levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief Logger class for NTP daemon
 * 
 * Provides thread-safe logging with configurable levels and output destinations.
 * Supports console, file, and syslog output.
 */
class Logger {
public:
    /**
     * @brief Constructor
     * @param log_file Path to log file (optional)
     * @param level Minimum log level to output
     * @param enable_console Whether to output to console
     */
    explicit Logger(const std::string& log_file = "", 
                   LogLevel level = LogLevel::INFO,
                   bool enable_console = true);
    
    /**
     * @brief Destructor
     */
    ~Logger();
    
    /**
     * @brief Set log level
     * @param level New log level
     */
    void setLevel(LogLevel level);
    
    /**
     * @brief Get current log level
     * @return Current log level
     */
    LogLevel getLevel() const;
    
    /**
     * @brief Enable/disable console output
     * @param enable Whether to enable console output
     */
    void setConsoleOutput(bool enable);
    
    /**
     * @brief Set log file
     * @param log_file Path to log file
     * @return true if successful, false otherwise
     */
    bool setLogFile(const std::string& log_file);
    
    /**
     * @brief Log debug message
     * @param message Message to log
     */
    void debug(const std::string& message);
    
    /**
     * @brief Log info message
     * @param message Message to log
     */
    void info(const std::string& message);
    
    /**
     * @brief Log warning message
     * @param message Message to log
     */
    void warning(const std::string& message);
    
    /**
     * @brief Log error message
     * @param message Message to log
     */
    void error(const std::string& message);
    
    /**
     * @brief Log fatal message
     * @param message Message to log
     */
    void fatal(const std::string& message);
    
    /**
     * @brief Log message with custom level
     * @param level Log level
     * @param message Message to log
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Get log level string representation
     * @param level Log level
     * @return String representation
     */
    static std::string levelToString(LogLevel level);
    
    /**
     * @brief Parse log level from string
     * @param level_str String representation of log level
     * @return Log level, defaults to INFO if invalid
     */
    static LogLevel stringToLevel(const std::string& level_str);

private:
    /**
     * @brief Internal logging method
     * @param level Log level
     * @param message Message to log
     */
    void logInternal(LogLevel level, const std::string& message);
    
    /**
     * @brief Format timestamp for logging
     * @return Formatted timestamp string
     */
    std::string formatTimestamp() const;
    
    /**
     * @brief Write to log file
     * @param message Formatted message to write
     */
    void writeToFile(const std::string& message);
    
    /**
     * @brief Write to console
     * @param message Formatted message to write
     */
    void writeToConsole(const std::string& message);

private:
    std::string log_file_path_;
    std::ofstream log_file_;
    LogLevel current_level_;
    bool console_output_enabled_;
    mutable std::mutex log_mutex_;
};

} // namespace simple_ntpd
