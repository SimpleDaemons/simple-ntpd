/**
 * @file test_ntp_config.cpp
 * @brief Unit tests for NTP configuration functionality
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/config_parser.hpp"
#include <cassert>
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
};

int main() {
  std::cout << "Running NTP Configuration Unit Tests..." << std::endl;
  return ConfigTestFramework::runTests();
}
