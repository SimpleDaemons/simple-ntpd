/**
 * @file net.hpp
 * @brief Network utility helpers
 */

#pragma once

#include <string>

namespace simple_ntpd {

/** @brief Check whether @p ip is contained in @p cidr (IPv4 only). */
bool isIpInCidr(const std::string &ip, const std::string &cidr);

} // namespace simple_ntpd
