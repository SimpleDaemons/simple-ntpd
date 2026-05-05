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
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <type_traits>

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
  authentication_algorithm = AuthAlgorithm::NONE;
  authentication_keys.clear();
  restrict_queries = false;
  allowed_clients = {"0.0.0.0/0"};
  denied_clients = {};
  enable_acl = false;
  enable_rate_limiting = false;
  connection_rate_limit_per_minute = 120;
  request_rate_limit_per_minute = 600;
  enable_ddos_protection = false;
  ddos_anomaly_threshold_per_second = 200;
  enable_encrypted_channels = false;
  enable_certificate_validation = false;
  enable_certificate_authentication = false;
  enable_tls = false;
  tls_cert_file = "";
  tls_key_file = "";
  tls_ca_file = "";

  worker_threads = 4;
  max_packet_size = 1024;
  enable_statistics = true;
  stats_interval = std::chrono::seconds(60);

  drift_file = "/var/lib/simple-ntpd/drift";
  enable_drift_compensation = true;

  leap_second_file = "/var/lib/simple-ntpd/leap-seconds.list";
  enable_leap_second_handling = true;

  enable_automatic_failover = false;
  enable_self_healing = false;
  enable_graceful_degradation = false;
  degradation_mode = DegradationMode::NORMAL;
  enable_state_persistence = false;
  state_file = "/var/lib/simple-ntpd/state.json";
  backup_config_file = "/var/lib/simple-ntpd/config.backup";
  service_restart_limit = 3;

  upstream_selection_algorithm = UpstreamSelectionAlgorithm::ROUND_ROBIN;
  enable_upstream_load_balancing = false;
  enable_upstream_failover = false;
  enable_dynamic_stratum_adjustment = false;
  enable_reference_clock_support = false;
  reference_clock_source = "local";
}

bool NtpConfig::loadFromFile(const std::string &config_file) {
  bool ok = parseConfigFile(config_file);
  if (ok) {
    setLastConfigFile(config_file);
    applyEnvironmentOverrides();
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

  applyEnvironmentOverrides();
  return true;
}

bool NtpConfig::validate() const {
  std::vector<std::string> errors;
  return validateDetailed(errors);
}

bool NtpConfig::validateDetailed(std::vector<std::string> &errors) const {
  errors.clear();

  if (listen_port < 1 || listen_port > 65535) {
    errors.push_back("listen_port must be in range 1-65535");
  }

  if (static_cast<int>(stratum) < 0 || static_cast<int>(stratum) > 15) {
    errors.push_back("stratum must be in range 0-15");
  }

  if (worker_threads < 1 || worker_threads > 64) {
    errors.push_back("worker_threads must be in range 1-64");
  }

  if (max_connections < 1 || max_connections > 100000) {
    errors.push_back("max_connections must be in range 1-100000");
  }

  if (max_packet_size < 48 || max_packet_size > 8192) {
    errors.push_back("max_packet_size must be in range 48-8192");
  }

  if (sync_interval.count() < 1 || sync_interval.count() > 86400) {
    errors.push_back("sync_interval must be in range 1-86400 seconds");
  }

  if (timeout.count() < 10 || timeout.count() > 60000) {
    errors.push_back("timeout must be in range 10-60000 milliseconds");
  }

  if (stats_interval.count() < 1 || stats_interval.count() > 3600) {
    errors.push_back("stats_interval must be in range 1-3600 seconds");
  }

  if (reference_id.size() != 4) {
    errors.push_back("reference_id must be exactly 4 characters");
  }

  if (log_max_size_bytes > 0 && log_max_size_bytes < 1024) {
    errors.push_back("log_max_size_bytes must be 0 or >= 1024");
  }

  if (log_max_files < 1 || log_max_files > 1000) {
    errors.push_back("log_max_files must be in range 1-1000");
  }

  if (enable_authentication && authentication_key.empty()) {
    errors.push_back("authentication_key is required when enable_authentication=true");
  }

  if (!enable_authentication && !authentication_key.empty()) {
    errors.push_back("authentication_key set while enable_authentication=false");
  }

  if (enable_leap_second_handling && leap_second_file.empty()) {
    errors.push_back("leap_second_file is required when enable_leap_second_handling=true");
  }

  if (enable_statistics && stats_interval.count() <= 0) {
    errors.push_back("stats_interval must be > 0 when enable_statistics=true");
  }

  if (restrict_queries && allowed_clients.empty()) {
    errors.push_back("allowed_clients must be set when restrict_queries=true");
  }

  if (enable_rate_limiting) {
    if (connection_rate_limit_per_minute == 0) {
      errors.push_back("connection_rate_limit_per_minute must be > 0 when rate limiting is enabled");
    }
    if (request_rate_limit_per_minute == 0) {
      errors.push_back("request_rate_limit_per_minute must be > 0 when rate limiting is enabled");
    }
  }

  if (enable_ddos_protection && ddos_anomaly_threshold_per_second == 0) {
    errors.push_back("ddos_anomaly_threshold_per_second must be > 0 when DDoS protection is enabled");
  }

  if ((enable_tls || enable_certificate_authentication) && (tls_cert_file.empty() || tls_key_file.empty())) {
    errors.push_back("tls_cert_file and tls_key_file are required when TLS/certificate authentication is enabled");
  }

  if (enable_certificate_validation && tls_ca_file.empty()) {
    errors.push_back("tls_ca_file is required when certificate validation is enabled");
  }

  if (enable_state_persistence && state_file.empty()) {
    errors.push_back("state_file is required when state persistence is enabled");
  }

  if (enable_automatic_failover && upstream_servers.empty()) {
    errors.push_back("upstream_servers must not be empty when automatic failover is enabled");
  }

  return errors.empty();
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
  ss << "  Auth Algorithm: " << static_cast<int>(authentication_algorithm) << "\n";
  ss << "  ACL Enabled: " << (enable_acl ? "Yes" : "No") << "\n";
  ss << "  Rate Limiting: " << (enable_rate_limiting ? "Yes" : "No") << "\n";
  ss << "  DDoS Protection: " << (enable_ddos_protection ? "Yes" : "No") << "\n";
  ss << "  TLS Enabled: " << (enable_tls ? "Yes" : "No") << "\n";
  ss << "  Cert Validation: " << (enable_certificate_validation ? "Yes" : "No") << "\n";
  ss << "  Cert Auth: " << (enable_certificate_authentication ? "Yes" : "No") << "\n";
  ss << "  Statistics: " << (enable_statistics ? "Yes" : "No") << "\n";
  ss << "  Drift Compensation: " << (enable_drift_compensation ? "Yes" : "No")
     << "\n";
  ss << "  Leap Second Handling: "
     << (enable_leap_second_handling ? "Yes" : "No") << "\n";
  ss << "  Automatic Failover: " << (enable_automatic_failover ? "Yes" : "No") << "\n";
  ss << "  Self Healing: " << (enable_self_healing ? "Yes" : "No") << "\n";
  ss << "  Graceful Degradation: " << (enable_graceful_degradation ? "Yes" : "No") << "\n";
  ss << "  Upstream Selection Algorithm: " << static_cast<int>(upstream_selection_algorithm) << "\n";
  ss << "  Dynamic Stratum: " << (enable_dynamic_stratum_adjustment ? "Yes" : "No") << "\n";
  ss << "  Reference Clock Support: " << (enable_reference_clock_support ? "Yes" : "No") << "\n";

  return ss.str();
}

bool NtpConfig::parseConfigFile(const std::string &config_file) {
  std::ifstream file(config_file);
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#' || line[0] == ';') {
      continue;
    }

    // Remove leading/trailing whitespace
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    // Check for section headers [section]
    if (line[0] == '[' && line[line.length() - 1] == ']') {
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
  } else if (lower_key == "authentication_algorithm" || lower_key == "auth_algorithm") {
    std::string alg = value;
    std::transform(alg.begin(), alg.end(), alg.begin(), ::tolower);
    if (alg == "md5") {
      authentication_algorithm = AuthAlgorithm::MD5;
    } else if (alg == "sha1") {
      authentication_algorithm = AuthAlgorithm::SHA1;
    } else if (alg == "sha256") {
      authentication_algorithm = AuthAlgorithm::SHA256;
    } else {
      authentication_algorithm = AuthAlgorithm::NONE;
    }
  } else if (lower_key == "authentication_keys" || lower_key == "auth_keys") {
    authentication_keys.clear();
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, ',')) {
      item.erase(0, item.find_first_not_of(" \t"));
      item.erase(item.find_last_not_of(" \t") + 1);
      if (!item.empty()) {
        parseAuthenticationKeySpec(item);
      }
    }
  } else if (lower_key == "restrict_queries" || lower_key == "restrict") {
    restrict_queries = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_acl" || lower_key == "acl") {
    enable_acl = (value == "true" || value == "1" || value == "yes");
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
  } else if (lower_key == "enable_rate_limiting" || lower_key == "rate_limit") {
    enable_rate_limiting = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "connection_rate_limit_per_minute" || lower_key == "conn_rate_limit") {
    try {
      connection_rate_limit_per_minute = static_cast<uint32_t>(std::stoul(value));
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "request_rate_limit_per_minute" || lower_key == "req_rate_limit") {
    try {
      request_rate_limit_per_minute = static_cast<uint32_t>(std::stoul(value));
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "enable_ddos_protection" || lower_key == "ddos_protection") {
    enable_ddos_protection = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "ddos_anomaly_threshold_per_second" || lower_key == "ddos_threshold") {
    try {
      ddos_anomaly_threshold_per_second = static_cast<uint32_t>(std::stoul(value));
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "enable_tls" || lower_key == "tls") {
    enable_tls = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_encrypted_channels" || lower_key == "encrypted_channels") {
    enable_encrypted_channels = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_certificate_validation" || lower_key == "cert_validation") {
    enable_certificate_validation = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_certificate_authentication" || lower_key == "cert_auth") {
    enable_certificate_authentication = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "tls_cert_file") {
    tls_cert_file = value;
  } else if (lower_key == "tls_key_file") {
    tls_key_file = value;
  } else if (lower_key == "tls_ca_file") {
    tls_ca_file = value;
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
  } else if (lower_key == "enable_automatic_failover" || lower_key == "auto_failover") {
    enable_automatic_failover = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_self_healing" || lower_key == "self_healing") {
    enable_self_healing = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_graceful_degradation" || lower_key == "graceful_degradation") {
    enable_graceful_degradation = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "degradation_mode") {
    std::string mode = value;
    std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
    if (mode == "reduced") {
      degradation_mode = DegradationMode::REDUCED_FUNCTIONALITY;
    } else if (mode == "prioritize_trusted") {
      degradation_mode = DegradationMode::PRIORITIZE_TRUSTED;
    } else {
      degradation_mode = DegradationMode::NORMAL;
    }
  } else if (lower_key == "enable_state_persistence" || lower_key == "state_persistence") {
    enable_state_persistence = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "state_file") {
    state_file = value;
  } else if (lower_key == "backup_config_file") {
    backup_config_file = value;
  } else if (lower_key == "service_restart_limit") {
    try {
      service_restart_limit = static_cast<uint32_t>(std::stoul(value));
    } catch (const std::exception &) {
      return false;
    }
  } else if (lower_key == "upstream_selection_algorithm" || lower_key == "upstream_algorithm") {
    std::string alg = value;
    std::transform(alg.begin(), alg.end(), alg.begin(), ::tolower);
    if (alg == "random") {
      upstream_selection_algorithm = UpstreamSelectionAlgorithm::RANDOM;
    } else if (alg == "least_errors") {
      upstream_selection_algorithm = UpstreamSelectionAlgorithm::LEAST_ERRORS;
    } else {
      upstream_selection_algorithm = UpstreamSelectionAlgorithm::ROUND_ROBIN;
    }
  } else if (lower_key == "enable_upstream_load_balancing" || lower_key == "upstream_load_balancing") {
    enable_upstream_load_balancing = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_upstream_failover" || lower_key == "upstream_failover") {
    enable_upstream_failover = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_dynamic_stratum_adjustment" || lower_key == "dynamic_stratum") {
    enable_dynamic_stratum_adjustment = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "enable_reference_clock_support" || lower_key == "reference_clock_support") {
    enable_reference_clock_support = (value == "true" || value == "1" || value == "yes");
  } else if (lower_key == "reference_clock_source") {
    reference_clock_source = value;
  }

  return true;
}

void NtpConfig::applyEnvironmentOverrides() {
  auto apply_bool = [](const char *name, bool &dst) {
    const char *v = std::getenv(name);
    if (!v) {
      return;
    }
    std::string s(v);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    dst = (s == "1" || s == "true" || s == "yes" || s == "on");
  };
  auto apply_int = [](const char *name, auto &dst) {
    const char *v = std::getenv(name);
    if (!v) {
      return;
    }
    try {
      using T = std::decay_t<decltype(dst)>;
      if constexpr (std::is_enum_v<T>) {
        dst = static_cast<T>(std::stoi(v));
      } else if constexpr (std::is_unsigned_v<T>) {
        dst = static_cast<T>(std::stoull(v));
      } else {
        dst = static_cast<T>(std::stoll(v));
      }
    } catch (const std::exception &) {
      // ignore invalid overrides
    }
  };
  auto apply_string = [](const char *name, std::string &dst) {
    const char *v = std::getenv(name);
    if (v) {
      dst = v;
    }
  };

  apply_string("SIMPLE_NTPD_LISTEN_ADDRESS", listen_address);
  apply_int("SIMPLE_NTPD_LISTEN_PORT", listen_port);
  apply_int("SIMPLE_NTPD_MAX_CONNECTIONS", max_connections);
  apply_int("SIMPLE_NTPD_WORKER_THREADS", worker_threads);
  apply_int("SIMPLE_NTPD_MAX_PACKET_SIZE", max_packet_size);
  apply_string("SIMPLE_NTPD_REFERENCE_ID", reference_id);
  apply_string("SIMPLE_NTPD_REFERENCE_CLOCK", reference_clock);
  apply_bool("SIMPLE_NTPD_ENABLE_STATISTICS", enable_statistics);
  {
    const char *v = std::getenv("SIMPLE_NTPD_STATS_INTERVAL_SEC");
    if (v) {
      try {
        stats_interval = std::chrono::seconds(std::stoll(v));
      } catch (const std::exception &) {
      }
    }
  }
  {
    const char *v = std::getenv("SIMPLE_NTPD_SYNC_INTERVAL_SEC");
    if (v) {
      try {
        sync_interval = std::chrono::seconds(std::stoll(v));
      } catch (const std::exception &) {
      }
    }
  }
  {
    const char *v = std::getenv("SIMPLE_NTPD_TIMEOUT_MS");
    if (v) {
      try {
        timeout = std::chrono::milliseconds(std::stoll(v));
      } catch (const std::exception &) {
      }
    }
  }
  apply_string("SIMPLE_NTPD_LOG_FILE", log_file);
  apply_int("SIMPLE_NTPD_LOG_LEVEL", log_level);
  apply_bool("SIMPLE_NTPD_LOG_JSON", log_json);
  apply_int("SIMPLE_NTPD_LOG_MAX_SIZE_BYTES", log_max_size_bytes);
  apply_int("SIMPLE_NTPD_LOG_MAX_FILES", log_max_files);
  apply_bool("SIMPLE_NTPD_ENABLE_SYSLOG", enable_syslog);
  apply_bool("SIMPLE_NTPD_ENABLE_CONSOLE_LOGGING", enable_console_logging);
  apply_bool("SIMPLE_NTPD_ENABLE_AUTHENTICATION", enable_authentication);
  apply_string("SIMPLE_NTPD_AUTH_KEY", authentication_key);
  apply_int("SIMPLE_NTPD_AUTH_ALGORITHM", authentication_algorithm);
  apply_bool("SIMPLE_NTPD_ENABLE_ACL", enable_acl);
  apply_bool("SIMPLE_NTPD_RESTRICT_QUERIES", restrict_queries);
  apply_bool("SIMPLE_NTPD_ENABLE_RATE_LIMITING", enable_rate_limiting);
  apply_int("SIMPLE_NTPD_CONN_RATE_LIMIT_PER_MIN", connection_rate_limit_per_minute);
  apply_int("SIMPLE_NTPD_REQ_RATE_LIMIT_PER_MIN", request_rate_limit_per_minute);
  apply_bool("SIMPLE_NTPD_ENABLE_DDOS_PROTECTION", enable_ddos_protection);
  apply_int("SIMPLE_NTPD_DDOS_THRESHOLD_PER_SEC", ddos_anomaly_threshold_per_second);
  apply_bool("SIMPLE_NTPD_ENABLE_TLS", enable_tls);
  apply_bool("SIMPLE_NTPD_ENABLE_ENCRYPTED_CHANNELS", enable_encrypted_channels);
  apply_bool("SIMPLE_NTPD_ENABLE_CERT_VALIDATION", enable_certificate_validation);
  apply_bool("SIMPLE_NTPD_ENABLE_CERT_AUTH", enable_certificate_authentication);
  apply_string("SIMPLE_NTPD_TLS_CERT_FILE", tls_cert_file);
  apply_string("SIMPLE_NTPD_TLS_KEY_FILE", tls_key_file);
  apply_string("SIMPLE_NTPD_TLS_CA_FILE", tls_ca_file);
  apply_bool("SIMPLE_NTPD_ENABLE_LEAP_SECOND_HANDLING", enable_leap_second_handling);
  apply_string("SIMPLE_NTPD_LEAP_SECOND_FILE", leap_second_file);
  apply_bool("SIMPLE_NTPD_ENABLE_AUTO_FAILOVER", enable_automatic_failover);
  apply_bool("SIMPLE_NTPD_ENABLE_SELF_HEALING", enable_self_healing);
  apply_bool("SIMPLE_NTPD_ENABLE_GRACEFUL_DEGRADATION", enable_graceful_degradation);
  apply_int("SIMPLE_NTPD_DEGRADATION_MODE", degradation_mode);
  apply_bool("SIMPLE_NTPD_ENABLE_STATE_PERSISTENCE", enable_state_persistence);
  apply_string("SIMPLE_NTPD_STATE_FILE", state_file);
  apply_string("SIMPLE_NTPD_BACKUP_CONFIG_FILE", backup_config_file);
  apply_int("SIMPLE_NTPD_SERVICE_RESTART_LIMIT", service_restart_limit);
  apply_int("SIMPLE_NTPD_UPSTREAM_SELECTION_ALGORITHM", upstream_selection_algorithm);
  apply_bool("SIMPLE_NTPD_ENABLE_UPSTREAM_LOAD_BALANCING", enable_upstream_load_balancing);
  apply_bool("SIMPLE_NTPD_ENABLE_UPSTREAM_FAILOVER", enable_upstream_failover);
  apply_bool("SIMPLE_NTPD_ENABLE_DYNAMIC_STRATUM", enable_dynamic_stratum_adjustment);
  apply_bool("SIMPLE_NTPD_ENABLE_REFERENCE_CLOCK_SUPPORT", enable_reference_clock_support);
  apply_string("SIMPLE_NTPD_REFERENCE_CLOCK_SOURCE", reference_clock_source);
}

bool NtpConfig::parseAuthenticationKeySpec(const std::string &spec) {
  size_t colon = spec.find(':');
  if (colon == std::string::npos || colon == 0 || colon == spec.size() - 1) {
    return false;
  }
  try {
    uint32_t key_id = static_cast<uint32_t>(std::stoul(spec.substr(0, colon)));
    std::string key_value = spec.substr(colon + 1);
    if (key_value.empty()) {
      return false;
    }
    authentication_keys[key_id] = key_value;
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

} // namespace simple_ntpd
