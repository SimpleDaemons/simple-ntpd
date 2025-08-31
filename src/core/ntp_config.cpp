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
    network_.listen_address = "0.0.0.0";
    network_.listen_port = 123;
    network_.enable_ipv6 = true;
    network_.max_connections = 1000;
    
    ntp_server_.stratum = 2;
    ntp_server_.reference_clock = "LOCAL";
    ntp_server_.upstream_servers = {"pool.ntp.org", "time.nist.gov"};
    ntp_server_.min_poll = 4;
    ntp_server_.max_poll = 17;
    
    logging_.level = LogLevel::INFO;
    logging_.destination = LogDestination::FILE;
    logging_.log_file = "/var/log/simple-ntpd/simple-ntpd.log";
    logging_.enable_syslog = true;
    logging_.syslog_facility = LOG_DAEMON;
    
    security_.enable_auth = false;
    security_.auth_key_file = "/etc/simple-ntpd/keys";
    security_.allowed_clients = {"0.0.0.0/0"};
    security_.denied_clients = {};
    security_.rate_limit = 100;
    
    performance_.worker_threads = 4;
    performance_.max_packet_size = 1024;
    performance_.connection_timeout = 30;
    performance_.enable_connection_pool = true;
    performance_.connection_pool_size = 100;
    
    drift_file_.enable = true;
    drift_file_.path = "/var/lib/simple-ntpd/drift";
    drift_file_.update_interval = 3600;
    
    leap_seconds_.enable = true;
    leap_seconds_.file_path = "/var/lib/simple-ntpd/leap-seconds.list";
    leap_seconds_.auto_update = true;
    leap_seconds_.update_url = "https://hpiers.obspm.fr/iers/bul/bulc/ntp/leap-seconds.list";
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
            network_.listen_address = value;
            return true;
        } else if (lower_key == "listen_port") {
            network_.listen_port = parseInt(value);
            return true;
        } else if (lower_key == "enable_ipv6") {
            network_.enable_ipv6 = parseBool(value);
            return true;
        } else if (lower_key == "max_connections") {
            int max_conn = parseInt(value);
            if (max_conn > 0) {
                network_.max_connections = max_conn;
                return true;
            }
        }
    }
    
    if (section == "ntp_server" || section.empty()) {
        if (lower_key == "stratum") {
            int stratum = parseInt(value);
            if (stratum >= 1 && stratum <= 15) {
                ntp_server_.stratum = stratum;
                return true;
            }
        } else if (lower_key == "reference_clock") {
            ntp_server_.reference_clock = value;
            return true;
        } else if (lower_key == "min_poll") {
            int poll = parseInt(value);
            if (poll >= 4 && poll <= 17) {
                ntp_server_.min_poll = poll;
                return true;
            }
        } else if (lower_key == "max_poll") {
            int poll = parseInt(value);
            if (poll >= 4 && poll <= 17) {
                ntp_server_.max_poll = poll;
                return true;
            }
        }
    }
    
    if (section == "logging" || section.empty()) {
        if (lower_key == "level") {
            std::string level = toLower(value);
            if (level == "debug") logging_.level = LogLevel::DEBUG;
            else if (level == "info") logging_.level = LogLevel::INFO;
            else if (level == "warning") logging_.level = LogLevel::WARNING;
            else if (level == "error") logging_.level = LogLevel::ERROR;
            else if (level == "fatal") logging_.level = LogLevel::FATAL;
            else return false;
            return true;
        } else if (lower_key == "destination") {
            std::string dest = toLower(value);
            if (dest == "console") logging_.destination = LogDestination::CONSOLE;
            else if (dest == "file") logging_.destination = LogDestination::FILE;
            else if (dest == "both") logging_.destination = LogDestination::BOTH;
            else return false;
            return true;
        } else if (lower_key == "log_file") {
            logging_.log_file = value;
            return true;
        }
    }
    
    return false;
}

bool NtpConfig::validate() const {
    bool valid = true;
    
    if (network_.listen_port < 1 || network_.listen_port > 65535) {
        Logger::getInstance().error("Invalid listen port: " + std::to_string(network_.listen_port));
        valid = false;
    }
    
    if (ntp_server_.stratum < 1 || ntp_server_.stratum > 15) {
        Logger::getInstance().error("Invalid stratum: " + std::to_string(ntp_server_.stratum));
        valid = false;
    }
    
    if (performance_.worker_threads < 1) {
        Logger::getInstance().error("Invalid worker threads: " + std::to_string(performance_.worker_threads));
        valid = false;
    }
    
    return valid;
}

} // namespace simple_ntpd
