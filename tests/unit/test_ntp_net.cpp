/**
 * @file test_ntp_net.cpp
 * @brief Unit tests for network helpers (ACL CIDR matching)
 */

#include "simple-ntpd/utils/net.hpp"
#include <cassert>
#include <iostream>

using namespace simple_ntpd;

int main() {
  std::cout << "Running NTP Network Utility Tests..." << std::endl;

  assert(isIpInCidr("10.1.2.3", "10.0.0.0/8"));
  assert(isIpInCidr("192.168.1.50", "192.168.1.0/24"));
  assert(!isIpInCidr("192.168.2.1", "192.168.1.0/24"));
  assert(isIpInCidr("127.0.0.1", "127.0.0.1"));
  assert(!isIpInCidr("127.0.0.2", "127.0.0.1"));
  assert(isIpInCidr("0.0.0.0", "0.0.0.0/0"));

  std::cout << "Network utility tests passed." << std::endl;
  return 0;
}
