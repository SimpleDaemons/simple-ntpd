/**
 * @file config_parser.hpp
 * @brief Configuration parser interface and factory
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple_ntpd/ntp_config.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace simple_ntpd {

/**
 * @brief Configuration file format types
 */
enum class ConfigFormat {
  INI,
  JSON,
  YAML,
  UNKNOWN
};

/**
 * @brief Configuration parser interface
 *
 * Abstract base class for parsing different configuration file formats
 */
class ConfigParser {
public:
  /**
   * @brief Constructor
   */
  ConfigParser() = default;

  /**
   * @brief Destructor
   */
  virtual ~ConfigParser() = default;

  /**
   * @brief Parse configuration from file
   * @param filename Path to configuration file
   * @param config Configuration object to populate
   * @return true if successful, false otherwise
   */
  virtual bool parseFile(const std::string &filename, NtpConfig &config) = 0;

  /**
   * @brief Parse configuration from string
   * @param content Configuration content
   * @param config Configuration object to populate
   * @return true if successful, false otherwise
   */
  virtual bool parseString(const std::string &content, NtpConfig &config) = 0;

  /**
   * @brief Get supported file extensions
   * @return Vector of supported extensions
   */
  virtual std::vector<std::string> getSupportedExtensions() const = 0;

  /**
   * @brief Get format name
   * @return Format name string
   */
  virtual std::string getFormatName() const = 0;

protected:
  /**
   * @brief Parse a key-value pair
   * @param key Configuration key
   * @param value Configuration value
   * @param config Configuration object to populate
   * @return true if parsed successfully, false otherwise
   */
  bool parseKeyValue(const std::string &key, const std::string &value, NtpConfig &config);

  /**
   * @brief Parse a list value (comma-separated)
   * @param value Comma-separated value string
   * @return Vector of individual values
   */
  std::vector<std::string> parseList(const std::string &value);

  /**
   * @brief Trim whitespace from string
   * @param str String to trim
   * @return Trimmed string
   */
  std::string trim(const std::string &str);

  /**
   * @brief Convert string to boolean
   * @param value String value
   * @return Boolean value
   */
  bool stringToBool(const std::string &value);

  /**
   * @brief Convert string to integer
   * @param value String value
   * @param result Output integer value
   * @return true if conversion successful, false otherwise
   */
  bool stringToInt(const std::string &value, int &result);

  /**
   * @brief Convert string to unsigned integer
   * @param value String value
   * @param result Output unsigned integer value
   * @return true if conversion successful, false otherwise
   */
  bool stringToUInt(const std::string &value, unsigned int &result);

  /**
   * @brief Convert string to size_t
   * @param value String value
   * @param result Output size_t value
   * @return true if conversion successful, false otherwise
   */
  bool stringToSizeT(const std::string &value, size_t &result);
};

/**
 * @brief Configuration parser factory
 *
 * Factory class for creating configuration parsers based on file format
 */
class ConfigParserFactory {
public:
  /**
   * @brief Create parser for specific format
   * @param format Configuration format
   * @return Parser instance
   */
  static std::unique_ptr<ConfigParser> createParser(ConfigFormat format);

  /**
   * @brief Create parser based on file extension
   * @param filename File name or path
   * @return Parser instance
   */
  static std::unique_ptr<ConfigParser> createParserFromFile(const std::string &filename);

  /**
   * @brief Detect format from file extension
   * @param filename File name or path
   * @return Detected format
   */
  static ConfigFormat detectFormat(const std::string &filename);

  /**
   * @brief Get all supported formats
   * @return Vector of supported formats
   */
  static std::vector<ConfigFormat> getSupportedFormats();

  /**
   * @brief Get format name
   * @param format Configuration format
   * @return Format name string
   */
  static std::string getFormatName(ConfigFormat format);
};

} // namespace simple_ntpd
