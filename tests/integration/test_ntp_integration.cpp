/**
 * @file test_ntp_integration.cpp
 * @brief Integration tests for NTP request/response flow
 */

#include "simple-ntpd/config/config.hpp"
#include "simple-ntpd/core/packet.hpp"
#include <cassert>
#include <iostream>

using namespace simple_ntpd;

int main() {
  std::cout << "Running NTP Integration Tests..." << std::endl;

  auto config = std::make_shared<NtpConfig>();
  assert(config->validate());

  NtpPacket req = NtpPacket::createClientRequest();
  assert(req.isValid());
  auto req_data = req.serializeToData();
  assert(req_data.size() == NTP_PACKET_SIZE);

  NtpPacket parsed_req;
  assert(parsed_req.parseFromData(req_data));
  assert(parsed_req.mode == static_cast<uint8_t>(NtpMode::CLIENT));

  NtpPacket resp = NtpPacket::createServerResponse(parsed_req, config->stratum, config->reference_id);
  assert(resp.mode == static_cast<uint8_t>(NtpMode::SERVER));
  assert(resp.isValid());
  auto resp_data = resp.serializeToData();
  assert(resp_data.size() == NTP_PACKET_SIZE);

  NtpPacket parsed_resp;
  assert(parsed_resp.parseFromData(resp_data));
  assert(parsed_resp.mode == static_cast<uint8_t>(NtpMode::SERVER));

  std::cout << "Integration tests passed." << std::endl;
  return 0;
}
