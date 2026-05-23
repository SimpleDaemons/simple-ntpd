/**
 * @file net.cpp
 * @brief Network utility helpers
 */

#include "simple-ntpd/utils/net.hpp"
#include <arpa/inet.h>
#include <cstdint>

namespace simple_ntpd {

bool isIpInCidr(const std::string &ip, const std::string &cidr) {
  const size_t slash = cidr.find('/');
  if (slash == std::string::npos) {
    return ip == cidr;
  }

  const std::string base_ip = cidr.substr(0, slash);
  const int prefix = std::stoi(cidr.substr(slash + 1));
  if (prefix < 0 || prefix > 32) {
    return false;
  }

  struct in_addr ip_addr {};
  struct in_addr base_addr {};
  if (inet_pton(AF_INET, ip.c_str(), &ip_addr) != 1 ||
      inet_pton(AF_INET, base_ip.c_str(), &base_addr) != 1) {
    return false;
  }

  const uint32_t ip_u = ntohl(ip_addr.s_addr);
  const uint32_t base_u = ntohl(base_addr.s_addr);
  const uint32_t mask = prefix == 0 ? 0 : 0xFFFFFFFFu << (32 - prefix);
  return (ip_u & mask) == (base_u & mask);
}

} // namespace simple_ntpd
