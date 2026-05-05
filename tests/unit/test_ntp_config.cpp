/**
 * @file test_ntp_config.cpp
 * @brief Unit tests for NTP configuration functionality
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ntpd/config/config.hpp"
#include "simple-ntpd/config/parser.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace simple_ntpd;

class ConfigTestFramework {
public:
  static int runTests() {
    int passed = 0;
    int total = 0;

    // Test basic configuration
    total++; if (testDefaultConfig()) { passed++; std::cout << "✓ testDefaultConfig passed" << std::endl; }
    else { std::cout << "✗ testDefaultConfig failed" << std::endl; }

    total++; if (testConfigValidation()) { passed++; std::cout << "✓ testConfigValidation passed" << std::endl; }
    else { std::cout << "✗ testConfigValidation failed" << std::endl; }

    total++; if (testIniConfigParsing()) { passed++; std::cout << "✓ testIniConfigParsing passed" << std::endl; }
    else { std::cout << "✗ testIniConfigParsing failed" << std::endl; }

    total++; if (testJsonConfigParsing()) { passed++; std::cout << "✓ testJsonConfigParsing passed" << std::endl; }
    else { std::cout << "✗ testJsonConfigParsing failed" << std::endl; }

    total++; if (testYamlConfigParsing()) { passed++; std::cout << "✓ testYamlConfigParsing passed" << std::endl; }
    else { std::cout << "✗ testYamlConfigParsing failed" << std::endl; }

    total++; if (testConfigFactory()) { passed++; std::cout << "✓ testConfigFactory passed" << std::endl; }
    else { std::cout << "✗ testConfigFactory failed" << std::endl; }

    total++; if (testDetailedValidation()) { passed++; std::cout << "✓ testDetailedValidation passed" << std::endl; }
    else { std::cout << "✗ testDetailedValidation failed" << std::endl; }

    total++; if (testEnvironmentOverrides()) { passed++; std::cout << "✓ testEnvironmentOverrides passed" << std::endl; }
    else { std::cout << "✗ testEnvironmentOverrides failed" << std::endl; }

    total++; if (testSecurityAndReliabilityOptions()) { passed++; std::cout << "✓ testSecurityAndReliabilityOptions passed" << std::endl; }
    else { std::cout << "✗ testSecurityAndReliabilityOptions failed" << std::endl; }

    std::cout << "\nConfig Test Results: " << passed << "/" << total << " tests passed" << std::endl;
    return (passed == total) ? 0 : 1;
  }

private:
  static bool testDefaultConfig() {
    try {
      NtpConfig config;
      assert(config.listen_address == "0.0.0.0");
      assert(config.listen_port == 123);
      assert(config.stratum == NtpStratum::SECONDARY_REFERENCE);
      assert(config.worker_threads == 4);
      assert(config.validate());
      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testConfigValidation() {
    try {
      NtpConfig config;
      
      // Test valid configuration
      assert(config.validate());

      // Test invalid port
      config.listen_port = 0;
      assert(!config.validate());
      config.listen_port = 123; // Reset

      // Test invalid stratum
      config.stratum = static_cast<NtpStratum>(99);
      assert(!config.validate());
      config.stratum = NtpStratum::SECONDARY_REFERENCE; // Reset

      // Test invalid worker threads
      config.worker_threads = 0;
      assert(!config.validate());
      config.worker_threads = 4; // Reset

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testIniConfigParsing() {
    try {
      std::string iniContent = R"(
# NTP Configuration
listen_address = 127.0.0.1
listen_port = 1234
stratum = 3
worker_threads = 8
log_level = 2
enable_console_logging = true
)";

      auto parser = ConfigParserFactory::createParser(ConfigFormat::INI);
      assert(parser != nullptr);

      NtpConfig config;
      assert(parser->parseString(iniContent, config));

      assert(config.listen_address == "127.0.0.1");
      assert(config.listen_port == 1234);
      assert(config.stratum == NtpStratum::TERTIARY_REFERENCE);
      assert(config.worker_threads == 8);
      assert(config.log_level == LogLevel::WARNING);
      assert(config.enable_console_logging == true);

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testJsonConfigParsing() {
    try {
      std::string jsonContent = R"({
  "listen_address": "192.168.1.1",
  "listen_port": 5678,
  "stratum": 2,
  "worker_threads": 6,
  "log_level": 1,
  "enable_console_logging": false
})";

      auto parser = ConfigParserFactory::createParser(ConfigFormat::JSON);
      assert(parser != nullptr);

      NtpConfig config;
      assert(parser->parseString(jsonContent, config));

      assert(config.listen_address == "192.168.1.1");
      assert(config.listen_port == 5678);
      assert(config.stratum == NtpStratum::SECONDARY_REFERENCE);
      assert(config.worker_threads == 6);
      assert(config.log_level == LogLevel::INFO);
      assert(config.enable_console_logging == false);

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testYamlConfigParsing() {
    try {
      std::string yamlContent = R"(
listen_address: 10.0.0.1
listen_port: 9999
stratum: 1
worker_threads: 12
log_level: 0
enable_console_logging: true
)";

      auto parser = ConfigParserFactory::createParser(ConfigFormat::YAML);
      assert(parser != nullptr);

      NtpConfig config;
      assert(parser->parseString(yamlContent, config));

      assert(config.listen_address == "10.0.0.1");
      assert(config.listen_port == 9999);
      assert(config.stratum == NtpStratum::PRIMARY_REFERENCE);
      assert(config.worker_threads == 12);
      assert(config.log_level == LogLevel::DEBUG);
      assert(config.enable_console_logging == true);

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testConfigFactory() {
    try {
      // Test format detection
      assert(ConfigParserFactory::detectFormat("config.ini") == ConfigFormat::INI);
      assert(ConfigParserFactory::detectFormat("config.json") == ConfigFormat::JSON);
      assert(ConfigParserFactory::detectFormat("config.yml") == ConfigFormat::YAML);
      assert(ConfigParserFactory::detectFormat("config.yaml") == ConfigFormat::YAML);
      assert(ConfigParserFactory::detectFormat("config.unknown") == ConfigFormat::UNKNOWN);

      // Test parser creation
      auto iniParser = ConfigParserFactory::createParser(ConfigFormat::INI);
      assert(iniParser != nullptr);
      assert(iniParser->getFormatName() == "INI");

      auto jsonParser = ConfigParserFactory::createParser(ConfigFormat::JSON);
      assert(jsonParser != nullptr);
      assert(jsonParser->getFormatName() == "JSON");

      auto yamlParser = ConfigParserFactory::createParser(ConfigFormat::YAML);
      assert(yamlParser != nullptr);
      assert(yamlParser->getFormatName() == "YAML");

      // Test file-based parser creation
      auto fileParser = ConfigParserFactory::createParserFromFile("test.ini");
      assert(fileParser != nullptr);

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testDetailedValidation() {
    try {
      NtpConfig config;
      config.enable_authentication = true;
      config.authentication_key.clear();

      std::vector<std::string> errors;
      assert(!config.validateDetailed(errors));
      assert(!errors.empty());

      config.authentication_key = "secret";
      errors.clear();
      assert(config.validateDetailed(errors));
      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testEnvironmentOverrides() {
    try {
      setenv("SIMPLE_NTPD_LISTEN_PORT", "9123", 1);
      setenv("SIMPLE_NTPD_LOG_JSON", "true", 1);
      setenv("SIMPLE_NTPD_WORKER_THREADS", "6", 1);
      setenv("SIMPLE_NTPD_REFERENCE_ID", "TEST", 1);

      NtpConfig config;
      assert(config.loadFromCommandLine(0, nullptr));
      assert(config.listen_port == 9123);
      assert(config.log_json);
      assert(config.worker_threads == 6);
      assert(config.reference_id == "TEST");

      unsetenv("SIMPLE_NTPD_LISTEN_PORT");
      unsetenv("SIMPLE_NTPD_LOG_JSON");
      unsetenv("SIMPLE_NTPD_WORKER_THREADS");
      unsetenv("SIMPLE_NTPD_REFERENCE_ID");
      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testSecurityAndReliabilityOptions() {
    try {
      NtpConfig config;
      assert(config.parseCommandLineArg("enable_acl", "true"));
      assert(config.parseCommandLineArg("enable_rate_limiting", "true"));
      assert(config.parseCommandLineArg("connection_rate_limit_per_minute", "50"));
      assert(config.parseCommandLineArg("request_rate_limit_per_minute", "250"));
      assert(config.parseCommandLineArg("enable_ddos_protection", "true"));
      assert(config.parseCommandLineArg("ddos_anomaly_threshold_per_second", "100"));
      assert(config.parseCommandLineArg("authentication_algorithm", "sha256"));
      assert(config.parseCommandLineArg("enable_tls", "true"));
      assert(config.parseCommandLineArg("tls_cert_file", "/tmp/cert.pem"));
      assert(config.parseCommandLineArg("tls_key_file", "/tmp/key.pem"));
      assert(config.parseCommandLineArg("enable_certificate_validation", "true"));
      assert(config.parseCommandLineArg("tls_ca_file", "/tmp/ca.pem"));
      assert(config.parseCommandLineArg("enable_automatic_failover", "true"));
      assert(config.parseCommandLineArg("enable_self_healing", "true"));
      assert(config.parseCommandLineArg("enable_graceful_degradation", "true"));
      assert(config.parseCommandLineArg("upstream_selection_algorithm", "random"));
      assert(config.parseCommandLineArg("enable_dynamic_stratum_adjustment", "true"));
      assert(config.parseCommandLineArg("enable_reference_clock_support", "true"));
      assert(config.parseCommandLineArg("reference_clock_source", "gps"));

      assert(config.enable_acl);
      assert(config.enable_rate_limiting);
      assert(config.connection_rate_limit_per_minute == 50);
      assert(config.request_rate_limit_per_minute == 250);
      assert(config.enable_ddos_protection);
      assert(config.ddos_anomaly_threshold_per_second == 100);
      assert(config.authentication_algorithm == NtpConfig::AuthAlgorithm::SHA256);
      assert(config.enable_tls);
      assert(config.tls_cert_file == "/tmp/cert.pem");
      assert(config.tls_key_file == "/tmp/key.pem");
      assert(config.enable_certificate_validation);
      assert(config.tls_ca_file == "/tmp/ca.pem");
      assert(config.enable_automatic_failover);
      assert(config.enable_self_healing);
      assert(config.enable_graceful_degradation);
      assert(config.upstream_selection_algorithm == NtpConfig::UpstreamSelectionAlgorithm::RANDOM);
      assert(config.enable_dynamic_stratum_adjustment);
      assert(config.enable_reference_clock_support);
      assert(config.reference_clock_source == "gps");
      return true;
    } catch (...) {
      return false;
    }
  }
};

int main() {
  std::cout << "Running NTP Configuration Unit Tests..." << std::endl;
  return ConfigTestFramework::runTests();
}
