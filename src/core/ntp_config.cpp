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

#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace simple_ntpd {

namespace {
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    bool parseBool(const std::string& value) {
        std::string lower = toLower(trim(value));
        return lower == "true" || lower == "yes" || lower == "1" || lower == "on";
    }

    int parseInt(const std::string& value) {
        try {
            return std::stoi(trim(value));
        } catch (const std::exception&) {
            return 0;
        }
    }
}

NtpConfig::NtpConfig() {
    setDefaults();
}

void NtpConfig::setDefaults() {
    listen_address = "0.0.0.0";
    listen_port = 123;
    enable_ipv6 = true;
    max_connections = 1000;
    
    stratum = NtpStratum::SECONDARY_REFERENCE;
    reference_clock = "LOCAL";
    upstream_servers = {"pool.ntp.org", "time.nist.gov"};
    sync_interval = std::chrono::seconds(64);
    timeout = std::chrono::milliseconds(1000);
    
    log_level = LogLevel::INFO;
    log_file = "/var/log/simple-ntpd/simple-ntpd.log";
    enable_console_logging = true;
    enable_syslog = true;
    
    enable_authentication = false;
    authentication_key = "";
    restrict_queries = false;
    allowed_clients = {"0.0.0.0/0"};
    denied_clients = {};
    
    worker_threads = 4;
    max_packet_size = 1024;
    enable_statistics = true;
    stats_interval = std::chrono::seconds(60);
    
    drift_file = "/var/lib/simple-ntpd/drift";
    enable_drift_compensation = true;
    
    leap_second_file = "/var/lib/simple-ntpd/leap-seconds.list";
    enable_leap_second_handling = true;
}

bool NtpConfig::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::getInstance().error("Failed to open configuration file: " + filename);
        return false;
    }

    std::string line;
    std::string current_section;
    int line_number = 0;
    bool success = true;

    while (std::getline(file, line)) {
        line_number++;
        line = trim(line);
        
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            
            if (!parseConfigValue(current_section, key, value)) {
                Logger::getInstance().warning("Invalid configuration at line " + 
                                           std::to_string(line_number) + ": " + line);
                success = false;
            }
        }
    }
    
    file.close();
    
    if (success) {
        Logger::getInstance().info("Configuration loaded successfully from: " + filename);
    }
    
    return success;
}

bool NtpConfig::parseConfigValue(const std::string& section, const std::string& key, const std::string& value) {
    std::string lower_key = toLower(key);
    
    if (section == "network" || section.empty()) {
        if (lower_key == "listen_address") {
            listen_address = value;
            return true;
        } else if (lower_key == "listen_port") {
            listen_port = parseInt(value);
            return true;
        } else if (lower_key == "enable_ipv6") {
            enable_ipv6 = parseBool(value);
            return true;
        } else if (lower_key == "max_connections") {
            int max_conn = parseInt(value);
            if (max_conn > 0) {
                max_connections = max_conn;
                return true;
            }
        }
    }
    
    if (section == "ntp_server" || section.empty()) {
        if (lower_key == "stratum") {
            int stratum_val = parseInt(value);
            if (stratum_val >= 1 && stratum_val <= 15) {
                stratum = static_cast<NtpStratum>(stratum_val);
                return true;
            }
        } else if (lower_key == "reference_clock") {
            reference_clock = value;
            return true;
        } else if (lower_key == "sync_interval") {
            int interval = parseInt(value);
            if (interval > 0) {
                sync_interval = std::chrono::seconds(interval);
                return true;
            }
        } else if (lower_key == "timeout") {
            int timeout_val = parseInt(value);
            if (timeout_val > 0) {
                timeout = std::chrono::milliseconds(timeout_val);
                return true;
            }
        }
    }
    
    if (section == "logging" || section.empty()) {
        if (lower_key == "level") {
            std::string level = toLower(value);
            if (level == "debug") log_level = LogLevel::DEBUG;
            else if (level == "info") log_level = LogLevel::INFO;
            else if (level == "warning") log_level = LogLevel::WARNING;
            else if (level == "error") log_level = LogLevel::ERROR;
            else if (level == "fatal") log_level = LogLevel::FATAL;
            else return false;
            return true;
        } else if (lower_key == "log_file") {
            log_file = value;
            return true;
        } else if (lower_key == "enable_console_logging") {
            enable_console_logging = parseBool(value);
            return true;
        } else if (lower_key == "enable_syslog") {
            enable_syslog = parseBool(value);
            return true;
        }
    }
    
    if (section == "performance" || section.empty()) {
        if (lower_key == "worker_threads") {
            int threads = parseInt(value);
            if (threads > 0) {
                worker_threads = threads;
                return true;
            }
        } else if (lower_key == "max_packet_size") {
            int size = parseInt(value);
            if (size > 0) {
                max_packet_size = size;
                return true;
            }
        }
    }
    
    return false;
}

bool NtpConfig::validate() const {
    bool valid = true;
    
    if (listen_port < 1 || listen_port > 65535) {
        Logger::getInstance().error("Invalid listen port: " + std::to_string(listen_port));
        valid = false;
    }
    
    if (static_cast<int>(stratum) < 1 || static_cast<int>(stratum) > 15) {
        Logger::getInstance().error("Invalid stratum: " + std::to_string(static_cast<int>(stratum)));
        valid = false;
    }
    
    if (worker_threads < 1) {
        Logger::getInstance().error("Invalid worker threads: " + std::to_string(worker_threads));
        valid = false;
    }
    
    return valid;
}

} // namespace simple_ntpd
