/**
 * @file config_parser.cpp
 * @brief Configuration parser implementation
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ntpd/config/parser.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace simple_ntpd {

// Forward declarations for concrete parser classes
class IniConfigParser;
class JsonConfigParser;
class YamlConfigParser;

bool ConfigParser::parseKeyValue(const std::string &key, const std::string &value, NtpConfig &config) {
  // Convert key to lowercase for case-insensitive comparison
  std::string lower_key = key;
  std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);

  if (lower_key == "listen_address" || lower_key == "bind_address") {
    config.listen_address = value;
  } else if (lower_key == "listen_port" || lower_key == "port") {
    int port;
    if (stringToInt(value, port)) {
      config.listen_port = static_cast<port_t>(port);
    }
  } else if (lower_key == "enable_ipv6" || lower_key == "ipv6") {
    config.enable_ipv6 = stringToBool(value);
  } else if (lower_key == "max_connections" || lower_key == "max_conn") {
    int max_conn;
    if (stringToInt(value, max_conn)) {
      config.max_connections = max_conn;
    }
  } else if (lower_key == "stratum") {
    int stratum_val;
    if (stringToInt(value, stratum_val) && stratum_val >= 0 && stratum_val <= 15) {
      config.stratum = static_cast<NtpStratum>(stratum_val);
    }
  } else if (lower_key == "reference_clock" || lower_key == "ref_clock") {
    config.reference_clock = value;
  } else if (lower_key == "reference_id" || lower_key == "ref_id") {
    config.reference_id = value;
  } else if (lower_key == "upstream_servers" || lower_key == "servers") {
    config.upstream_servers = parseList(value);
  } else if (lower_key == "sync_interval") {
    int seconds;
    if (stringToInt(value, seconds)) {
      config.sync_interval = std::chrono::seconds(seconds);
    }
  } else if (lower_key == "timeout") {
    int ms;
    if (stringToInt(value, ms)) {
      config.timeout = std::chrono::milliseconds(ms);
    }
  } else if (lower_key == "log_level" || lower_key == "loglevel") {
    int level;
    if (stringToInt(value, level) && level >= 0 && level <= 4) {
      config.log_level = static_cast<LogLevel>(level);
    }
  } else if (lower_key == "log_file" || lower_key == "logfile") {
    config.log_file = value;
  } else if (lower_key == "enable_console_logging" || lower_key == "console_log") {
    config.enable_console_logging = stringToBool(value);
  } else if (lower_key == "enable_syslog" || lower_key == "syslog") {
    config.enable_syslog = stringToBool(value);
  } else if (lower_key == "enable_authentication" || lower_key == "auth") {
    config.enable_authentication = stringToBool(value);
  } else if (lower_key == "authentication_key" || lower_key == "auth_key") {
    config.authentication_key = value;
  } else if (lower_key == "restrict_queries" || lower_key == "restrict") {
    config.restrict_queries = stringToBool(value);
  } else if (lower_key == "allowed_clients" || lower_key == "allow") {
    config.allowed_clients = parseList(value);
  } else if (lower_key == "denied_clients" || lower_key == "deny") {
    config.denied_clients = parseList(value);
  } else if (lower_key == "worker_threads" || lower_key == "threads") {
    size_t threads;
    if (stringToSizeT(value, threads) && threads >= 1 && threads <= 64) {
      config.worker_threads = threads;
    }
  } else if (lower_key == "max_packet_size" || lower_key == "packet_size") {
    size_t size;
    if (stringToSizeT(value, size) && size >= 48 && size <= 8192) {
      config.max_packet_size = size;
    }
  } else if (lower_key == "enable_statistics" || lower_key == "stats") {
    config.enable_statistics = stringToBool(value);
  } else if (lower_key == "stats_interval") {
    int seconds;
    if (stringToInt(value, seconds)) {
      config.stats_interval = std::chrono::seconds(seconds);
    }
  } else if (lower_key == "drift_file" || lower_key == "drift") {
    config.drift_file = value;
  } else if (lower_key == "enable_drift_compensation" || lower_key == "drift_comp") {
    config.enable_drift_compensation = stringToBool(value);
  } else if (lower_key == "leap_second_file" || lower_key == "leap_seconds") {
    config.leap_second_file = value;
  } else if (lower_key == "enable_leap_second_handling" || lower_key == "leap_handling") {
    config.enable_leap_second_handling = stringToBool(value);
  } else {
    // Unknown key, ignore
    return false;
  }

  return true;
}

std::vector<std::string> ConfigParser::parseList(const std::string &value) {
  std::vector<std::string> result;
  std::stringstream ss(value);
  std::string item;

  while (std::getline(ss, item, ',')) {
    item = trim(item);
    if (!item.empty()) {
      result.push_back(item);
    }
  }

  return result;
}

std::string ConfigParser::trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

bool ConfigParser::stringToBool(const std::string &value) {
  std::string lower_value = value;
  std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);
  
  return (lower_value == "true" || lower_value == "1" || lower_value == "yes" || 
          lower_value == "on" || lower_value == "enabled");
}

bool ConfigParser::stringToInt(const std::string &value, int &result) {
  try {
    result = std::stoi(value);
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool ConfigParser::stringToUInt(const std::string &value, unsigned int &result) {
  try {
    result = static_cast<unsigned int>(std::stoul(value));
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool ConfigParser::stringToSizeT(const std::string &value, size_t &result) {
  try {
    result = std::stoul(value);
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

// IniConfigParser implementation
class IniConfigParser : public ConfigParser {
public:
  bool parseFile(const std::string &filename, NtpConfig &config) override {
    std::ifstream file(filename);
    if (!file.is_open()) {
      return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return parseString(content, config);
  }

  bool parseString(const std::string &content, NtpConfig &config) override {
    std::istringstream stream(content);
    std::string line;
    int line_number = 0;

    while (std::getline(stream, line)) {
      line_number++;
      line = trim(line);

      // Skip empty lines and comments
      if (line.empty() || line[0] == '#' || line[0] == ';') {
        continue;
      }

      // Parse key=value pairs
      size_t equal_pos = line.find('=');
      if (equal_pos != std::string::npos) {
        std::string key = trim(line.substr(0, equal_pos));
        std::string value = trim(line.substr(equal_pos + 1));
        parseKeyValue(key, value, config);
      }
    }

    return true;
  }

  std::vector<std::string> getSupportedExtensions() const override {
    return {"ini", "conf", "cfg"};
  }

  std::string getFormatName() const override {
    return "INI";
  }
};

// JsonConfigParser implementation
class JsonConfigParser : public ConfigParser {
public:
  bool parseFile(const std::string &filename, NtpConfig &config) override {
    std::ifstream file(filename);
    if (!file.is_open()) {
      return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return parseString(content, config);
  }

  bool parseString(const std::string &content, NtpConfig &config) override {
    // For now, implement a simple JSON parser
    // In a production system, you'd use a proper JSON library like nlohmann/json
    // This is a basic implementation for demonstration
    
    // Simple key-value extraction from JSON-like content
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
      line = trim(line);
      
      // Skip empty lines and comments
      if (line.empty() || line[0] == '/' || line[0] == '*') {
        continue;
      }
      
      // Look for "key": "value" patterns
      size_t colon_pos = line.find(':');
      if (colon_pos != std::string::npos) {
        std::string key = trim(line.substr(0, colon_pos));
        std::string value = trim(line.substr(colon_pos + 1));
        
        // Remove quotes and commas
        if (key.length() >= 2 && key.front() == '"' && key.back() == '"') {
          key = key.substr(1, key.length() - 2);
        }
        if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
          value = value.substr(1, value.length() - 2);
        }
        if (value.length() > 0 && value.back() == ',') {
          value = value.substr(0, value.length() - 1);
        }
        
        // Additional cleanup for JSON - remove any remaining quotes
        if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
          value = value.substr(1, value.length() - 2);
        }
        
        parseKeyValue(key, value, config);
      }
    }
    
    return true;
  }

  std::vector<std::string> getSupportedExtensions() const override {
    return {"json"};
  }

  std::string getFormatName() const override {
    return "JSON";
  }
};

// YamlConfigParser implementation
class YamlConfigParser : public ConfigParser {
public:
  bool parseFile(const std::string &filename, NtpConfig &config) override {
    std::ifstream file(filename);
    if (!file.is_open()) {
      return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return parseString(content, config);
  }

  bool parseString(const std::string &content, NtpConfig &config) override {
    // For now, implement a simple YAML parser
    // In a production system, you'd use a proper YAML library like yaml-cpp
    // This is a basic implementation for demonstration
    
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
      line = trim(line);
      
      // Skip empty lines and comments
      if (line.empty() || line[0] == '#') {
        continue;
      }
      
      // Look for key: value patterns
      size_t colon_pos = line.find(':');
      if (colon_pos != std::string::npos) {
        std::string key = trim(line.substr(0, colon_pos));
        std::string value = trim(line.substr(colon_pos + 1));
        
        parseKeyValue(key, value, config);
      }
    }
    
    return true;
  }

  std::vector<std::string> getSupportedExtensions() const override {
    return {"yml", "yaml"};
  }

  std::string getFormatName() const override {
    return "YAML";
  }
};

// ConfigParserFactory implementation
std::unique_ptr<ConfigParser> ConfigParserFactory::createParser(ConfigFormat format) {
  switch (format) {
    case ConfigFormat::INI:
      return std::make_unique<IniConfigParser>();
    case ConfigFormat::JSON:
      return std::make_unique<JsonConfigParser>();
    case ConfigFormat::YAML:
      return std::make_unique<YamlConfigParser>();
    default:
      return nullptr;
  }
}

std::unique_ptr<ConfigParser> ConfigParserFactory::createParserFromFile(const std::string &filename) {
  ConfigFormat format = detectFormat(filename);
  return createParser(format);
}

ConfigFormat ConfigParserFactory::detectFormat(const std::string &filename) {
  size_t dot_pos = filename.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return ConfigFormat::UNKNOWN;
  }

  std::string extension = filename.substr(dot_pos + 1);
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

  if (extension == "ini" || extension == "conf") {
    return ConfigFormat::INI;
  } else if (extension == "json") {
    return ConfigFormat::JSON;
  } else if (extension == "yml" || extension == "yaml") {
    return ConfigFormat::YAML;
  }

  return ConfigFormat::UNKNOWN;
}

std::vector<ConfigFormat> ConfigParserFactory::getSupportedFormats() {
  return {ConfigFormat::INI, ConfigFormat::JSON, ConfigFormat::YAML};
}

std::string ConfigParserFactory::getFormatName(ConfigFormat format) {
  switch (format) {
    case ConfigFormat::INI:
      return "INI";
    case ConfigFormat::JSON:
      return "JSON";
    case ConfigFormat::YAML:
      return "YAML";
    default:
      return "UNKNOWN";
  }
}

} // namespace simple_ntpd
