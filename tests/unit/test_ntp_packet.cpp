/**
 * @file test_ntp_packet.cpp
 * @brief Unit tests for NTP packet functionality
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple_ntpd/ntp_packet.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace simple_ntpd;

// Simple test framework
class TestFramework {
public:
  static int runTests() {
    int passed = 0;
    int total = 0;

    // Test NTP packet creation
    total++; if (testPacketCreation()) { passed++; std::cout << "✓ testPacketCreation passed" << std::endl; }
    else { std::cout << "✗ testPacketCreation failed" << std::endl; }

    total++; if (testPacketValidation()) { passed++; std::cout << "✓ testPacketValidation passed" << std::endl; }
    else { std::cout << "✗ testPacketValidation failed" << std::endl; }

    total++; if (testPacketSerialization()) { passed++; std::cout << "✓ testPacketSerialization passed" << std::endl; }
    else { std::cout << "✗ testPacketSerialization failed" << std::endl; }

    total++; if (testTimestampConversion()) { passed++; std::cout << "✓ testTimestampConversion passed" << std::endl; }
    else { std::cout << "✗ testTimestampConversion failed" << std::endl; }

    total++; if (testClientRequestCreation()) { passed++; std::cout << "✓ testClientRequestCreation passed" << std::endl; }
    else { std::cout << "✗ testClientRequestCreation failed" << std::endl; }

    total++; if (testServerResponseCreation()) { passed++; std::cout << "✓ testServerResponseCreation passed" << std::endl; }
    else { std::cout << "✗ testServerResponseCreation failed" << std::endl; }

    std::cout << "\nTest Results: " << passed << "/" << total << " tests passed" << std::endl;
    return (passed == total) ? 0 : 1;
  }

private:
  static bool testPacketCreation() {
    try {
      NtpPacket packet;
      assert(packet.version == NTP_VERSION);
      assert(packet.stratum == 0);
      assert(packet.mode == 0);
      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testPacketValidation() {
    try {
      // Test valid packet
      NtpPacket validPacket;
      validPacket.version = NTP_VERSION;
      validPacket.mode = static_cast<uint8_t>(NtpMode::CLIENT);
      validPacket.stratum = 2;
      validPacket.poll = 4;
      validPacket.precision = -6;
      validPacket.leap_indicator = 0;

      assert(validPacket.isValid());

      // Test detailed validation
      std::vector<std::string> errors;
      assert(validPacket.validateDetailed(errors));
      assert(errors.empty());

      // Test invalid packet
      NtpPacket invalidPacket;
      invalidPacket.version = 3; // Valid version but will test other invalid fields
      invalidPacket.mode = 3;    // Valid mode but will test other invalid fields
      invalidPacket.stratum = 99; // Invalid stratum

      assert(!invalidPacket.isValid());
      assert(!invalidPacket.validateDetailed(errors));
      assert(!errors.empty());

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testPacketSerialization() {
    try {
      NtpPacket packet;
      packet.version = NTP_VERSION;
      packet.mode = static_cast<uint8_t>(NtpMode::CLIENT);
      packet.stratum = 2;

      auto data = packet.serializeToData();
      assert(data.size() == NTP_PACKET_SIZE);

      NtpPacket parsedPacket;
      assert(parsedPacket.parseFromData(data));
      assert(parsedPacket.version == packet.version);
      assert(parsedPacket.mode == packet.mode);
      assert(parsedPacket.stratum == packet.stratum);

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testTimestampConversion() {
    try {
      auto now = std::chrono::system_clock::now();
      auto ntpTime = NtpTimestamp::fromSystemTime(now);
      auto backToSystem = ntpTime.toSystemTime();

      // Allow for small precision differences
      auto diff = std::chrono::duration_cast<std::chrono::microseconds>(
          now - backToSystem).count();
      assert(std::abs(diff) < 1000); // Within 1ms

      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testClientRequestCreation() {
    try {
      auto packet = NtpPacket::createClientRequest();
      assert(packet.mode == static_cast<uint8_t>(NtpMode::CLIENT));
      assert(packet.version == NTP_VERSION);
      assert(packet.stratum == 0);
      assert(packet.isValid());
      return true;
    } catch (...) {
      return false;
    }
  }

  static bool testServerResponseCreation() {
    try {
      auto clientPacket = NtpPacket::createClientRequest();
      auto response = NtpPacket::createServerResponse(
          clientPacket, NtpStratum::SECONDARY_REFERENCE, "LOCL");

      assert(response.mode == static_cast<uint8_t>(NtpMode::SERVER));
      assert(response.version == NTP_VERSION);
      assert(response.stratum == static_cast<uint8_t>(NtpStratum::SECONDARY_REFERENCE));
      assert(response.isValid());
      return true;
    } catch (...) {
      return false;
    }
  }
};

int main() {
  std::cout << "Running NTP Packet Unit Tests..." << std::endl;
  return TestFramework::runTests();
}
