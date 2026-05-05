/**
 * @file test_ntp_performance.cpp
 * @brief Basic performance smoke tests for packet processing
 */

#include "simple-ntpd/core/packet.hpp"
#include <cassert>
#include <chrono>
#include <iostream>

using namespace simple_ntpd;

int main() {
  std::cout << "Running NTP Performance Tests..." << std::endl;

  constexpr int kIterations = 20000;
  auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < kIterations; ++i) {
    NtpPacket req = NtpPacket::createClientRequest();
    auto data = req.serializeToData();
    NtpPacket parsed;
    assert(parsed.parseFromData(data));
    NtpPacket resp =
        NtpPacket::createServerResponse(parsed, NtpStratum::SECONDARY_REFERENCE, "LOCL");
    auto resp_data = resp.serializeToData();
    assert(resp_data.size() == NTP_PACKET_SIZE);
  }

  auto end = std::chrono::steady_clock::now();
  auto elapsed_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "Processed " << kIterations << " synthetic request/response cycles in "
            << elapsed_us << " us" << std::endl;

  // Sanity threshold to catch severe regressions in CI.
  assert(elapsed_us > 0);
  assert(elapsed_us < 5'000'000);

  std::cout << "Performance tests passed." << std::endl;
  return 0;
}
