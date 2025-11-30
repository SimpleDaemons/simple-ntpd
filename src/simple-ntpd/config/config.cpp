/**
 * @file ntp_config.cpp
 * @brief NTP server configuration implementation
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ntpd/config/config.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace simple_ntpd {

// NtpConfig implementation
NtpConfig::NtpConfig() { setDefaults(); }

void NtpConfig::setDefaults() {
  listen_address = "0.0.0.0";
  listen_port = 123;
  enable_ipv6 = true;
  max_connections = 1000;

  stratum = NtpStratum::SECONDARY_REFERENCE;
  reference_clock = "LOCAL";
  reference_id = "LOCL";
  upstream_servers = {"pool.ntp.org", "time.nist.gov"};
  sync_interval = std::chrono::seconds(64);
  timeout = std::chrono::milliseconds(1000);

  log_level = LogLevel::INFO;
  log_file = "/var/log/simple-ntpd/simple-ntpd.log";
  enable_console_logging = true;
  enable_syslog = true;
  log_json = false;
  log_max_size_bytes = 0;
  log_max_files = 5;

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

bool NtpConfig::loadFromFile(const std::string &config_file) {
  bool ok = parseConfigFile(config_file);
  if (ok) {
    setLastConfigFile(config_file);
  }
  return ok;
}

bool NtpConfig::loadFromCommandLine(int argc, char *argv[]) {
  // Simple command line parsing for now
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg.substr(0, 2) == "--") {
      size_t pos = arg.find('=');
      if (pos != std::string::npos) {
        std::string key = arg.substr(2, pos - 2);
        std::string value = arg.substr(pos + 1);
        parseCommandLineArg(key, value);
      }
    }
  }

  return true;
}

bool NtpConfig::validate() const {
  // Validate listen port
  if (listen_port < 1 || listen_port > 65535) {
    return false;
  }

  // Validate stratum
  if (static_cast<int>(stratum) < 0 || static_cast<int>(stratum) > 15) {
    return false;
  }

  // Validate worker threads
  if (worker_threads < 1 || worker_threads > 64) {
    return false;
  }

  // Validate max connections
  if (max_connections < 1 || max_connections > 100000) {
    return false;
  }

  // Validate max packet size
  if (max_packet_size < 48 || max_packet_size > 8192) {
    return false;
  }

  return true;
}

std::string NtpConfig::toString() const {
  std::stringstream ss;
  ss << "NTP Configuration:\n";
  ss << "  Listen Address: " << listen_address << ":" << listen_port << "\n";
  ss << "  IPv6 Enabled: " << (enable_ipv6 ? "Yes" : "No") << "\n";
  ss << "  Max Connections: " << max_connections << "\n";
  ss << "  Stratum: " << static_cast<int>(stratum) << "\n";
  ss << "  Reference Clock: " << reference_clock << "\n";
  ss << "  Reference ID: " << reference_id << "\n";
  ss << "  Worker Threads: " << worker_threads << "\n";
  ss << "  Log Level: " << static_cast<int>(log_level) << "\n";
  ss << "  Log File: " << log_file << "\n";
  ss << "  Console Logging: " << (enable_console_logging ? "Yes" : "No")
     << "\n";
  ss << "  Syslog: " << (enable_syslog ? "Yes" : "No") << "\n";
  ss << "  Authentication: " << (enable_authentication ? "Yes" : "No") << "\n";
  ss << "  Statistics: " << (enable_statistics ? "Yes" : "No") << "\n";
  ss << "  Drift Compensation: " << (enable_drift_compensation ? "Yes" : "No")
     << "\n";
  ss << "  Leap Second Handling: "
     << (enable_leap_second_handling ? "Yes" : "No") << "\n";

  return ss.str();
}

bool NtpConfig::parseConfigFile(const std::string &config_file) {
  std::ifstream file(config_file);
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  std::string current_section;
  int line_number = 0;

  while (std::getline(file, line)) {
    line_number++;

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#' || line[0] == ';') {
      continue;
    }

    // Remove leading/trailing whitespace
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    // Check for section headers [section]
    if (line[0] == '[' && line[line.length() - 1] == ']') {
      current_section = line.substr(1, line.length() - 2);
      continue;
    }

    // Parse key=value pairs
    size_t pos = line.find('=');
    if (pos != std::string::npos) {
      std::string key = line.substr(0, pos);
      std::string value = line.substr(pos + 1);

      // Remove quotes from value if present
      if (value.length() >= 2 && value[0] == '"' &&
          value[value.length() - 1] == '"') {
        value = value.substr(1, value.length() - 2);
      }

      // Parse the configuration value
      if (!parseCommandLineArg(key, value)) {
        // Log warning about invalid configuration
        continue;
      }
    }
  }

  return true;
}

bool NtpConfig::parseCommandLineArg(const std::string &key,
                                    const std::string &value) {
  // Convert key to lowercase for case-insensitive comparison
  std::string lower_key = key;
  std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                 ::tolower);

  if (lower_key == "listen_address" || lower_key == "bind_address") {
    listen_address = value;
  } else if (lower_key == "listen_port" || lower_key == "port") {
    try {
      listen_port = static_cast<port_t>(std::stoi(value));
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "enable_ipv6" || lower_key == "ipv6") {
    enable_ipv6 = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "max_connections" || lower_key == "max_conn") {
    try {
      max_connections = std::stoi(value);
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "stratum") {
    try {
      int stratum_val = std::stoi(value);
      if (stratum_val >= 0 && stratum_val <= 15) {
        stratum = static_cast<NtpStratum>(stratum_val);
      }
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "reference_clock" || lower_key == "ref_clock") {
    reference_clock = value;
  } else if (lower_key == "reference_id" || lower_key == "ref_id") {
    reference_id = value;
  } else if (lower_key == "upstream_servers" || lower_key == "servers") {
    // Parse comma-separated list
    std::stringstream ss(value);
    std::string server;
    upstream_servers.clear();
    while (std::getline(ss, server, ',')) {
      // Remove whitespace
      server.erase(0, server.find_first_not_of(" \t"));
      server.erase(server.find_last_not_of(" \t") + 1);
      if (!server.empty()) {
        upstream_servers.push_back(server);
      }
    }
  } else if (lower_key == "sync_interval") {
    try {
      int seconds = std::stoi(value);
      sync_interval = std::chrono::seconds(seconds);
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "timeout") {
    try {
      int ms = std::stoi(value);
      timeout = std::chrono::milliseconds(ms);
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "log_level" || lower_key == "loglevel") {
    try {
      int level = std::stoi(value);
      if (level >= 0 && level <= 4) {
        log_level = static_cast<LogLevel>(level);
      }
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "log_file" || lower_key == "logfile") {
    log_file = value;
  } else if (lower_key == "enable_console_logging" ||
             lower_key == "console_log") {
    enable_console_logging =
        (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_syslog" || lower_key == "syslog") {
    enable_syslog = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "log_json" || lower_key == "json_logs") {
    log_json = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "log_max_size_bytes" || lower_key == "log_max_size") {
    try {
      log_max_size_bytes = static_cast<uint64_t>(std::stoull(value));
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "log_max_files") {
    try {
      log_max_files = static_cast<uint32_t>(std::stoul(value));
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "enable_authentication" || lower_key == "auth") {
    enable_authentication = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "authentication_key" || lower_key == "auth_key") {
    authentication_key = value;
  } else if (lower_key == "restrict_queries" || lower_key == "restrict") {
    restrict_queries = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "allowed_clients" || lower_key == "allow") {
    // Parse comma-separated list
    std::stringstream ss(value);
    std::string client;
    allowed_clients.clear();
    while (std::getline(ss, client, ',')) {
      client.erase(0, client.find_first_not_of(" \t"));
      client.erase(client.find_last_not_of(" \t") + 1);
      if (!client.empty()) {
        allowed_clients.push_back(client);
      }
    }
  } else if (lower_key == "denied_clients" || lower_key == "deny") {
    // Parse comma-separated list
    std::stringstream ss(value);
    std::string client;
    denied_clients.clear();
    while (std::getline(ss, client, ',')) {
      client.erase(0, client.find_first_not_of(" \t"));
      client.erase(client.find_last_not_of(" \t") + 1);
      if (!client.empty()) {
        denied_clients.push_back(client);
      }
    }
  } else if (lower_key == "worker_threads" || lower_key == "threads") {
    try {
      size_t threads = std::stoul(value);
      if (threads >= 1 && threads <= 64) {
        worker_threads = threads;
      }
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "max_packet_size" || lower_key == "packet_size") {
    try {
      size_t size = std::stoul(value);
      if (size >= 48 && size <= 8192) {
        max_packet_size = size;
      }
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "enable_statistics" || lower_key == "stats") {
    enable_statistics = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "stats_interval") {
    try {
      int seconds = std::stoi(value);
      stats_interval = std::chrono::seconds(seconds);
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "drift_file" || lower_key == "drift") {
    drift_file = value;
  } else if (lower_key == "enable_drift_compensation" ||
             lower_key == "drift_comp") {
    enable_drift_compensation =
        (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "leap_second_file" || lower_key == "leap_seconds") {
    leap_second_file = value;
  } else if (lower_key == "enable_leap_second_handling" ||
             lower_key == "leap_handling") {
    enable_leap_second_handling =
        (value == "true" || value == "1" || value == "yes");
  }

  return true;
}

} // namespace simple_ntpd
