/**
 * @file test_ntp_security.cpp
 * @brief Security-focused tests for authentication/ACL/rate-limit config
 */

#include "simple-ntpd/config/config.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace simple_ntpd;

int main() {
  std::cout << "Running NTP Security Tests..." << std::endl;

  NtpConfig config;
  config.enable_authentication = true;
  config.authentication_algorithm = NtpConfig::AuthAlgorithm::SHA256;
  config.authentication_key = "test-secret";
  config.enable_acl = true;
  config.restrict_queries = true;
  config.allowed_clients = {"10.0.0.0/8"};
  config.enable_rate_limiting = true;
  config.connection_rate_limit_per_minute = 100;
  config.request_rate_limit_per_minute = 500;
  config.enable_ddos_protection = true;
  config.ddos_anomaly_threshold_per_second = 50;

  std::vector<std::string> errors;
  assert(config.validateDetailed(errors));
  assert(errors.empty());

  config.authentication_key.clear();
  assert(!config.validateDetailed(errors));
  assert(!errors.empty());

  std::cout << "Security tests passed." << std::endl;
  return 0;
}
