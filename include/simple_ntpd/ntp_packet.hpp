/*
 * Copyright 2025 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/platform.hpp"
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace simple_ntpd {

/**
 * @brief NTP timestamp structure
 *
 * Represents an NTP timestamp with seconds and fraction parts
 */
struct NtpTimestamp {
  uint32_t seconds;  // Seconds since NTP epoch (1900-01-01)
  uint32_t fraction; // Fraction of a second (2^-32)

  NtpTimestamp() : seconds(0), fraction(0) {}
  NtpTimestamp(uint32_t sec, uint32_t frac) : seconds(sec), fraction(frac) {}

  /**
   * @brief Convert to system time
   * @return System time point
   */
  std::chrono::system_clock::time_point toSystemTime() const;

  /**
   * @brief Create from system time
   * @param time System time point
   * @return NTP timestamp
   */
  static NtpTimestamp
  fromSystemTime(const std::chrono::system_clock::time_point &time);

  /**
   * @brief Get current NTP time
   * @return Current NTP timestamp
   */
  static NtpTimestamp now();
};

/**
 * @brief NTP packet structure
 *
 * Represents a complete NTP packet according to RFC 5905
 */
struct NtpPacket {
  // First word (32 bits)
  uint8_t leap_indicator : 2; // Leap indicator
  uint8_t version : 3;        // Version number
  uint8_t mode : 3;           // Mode
  uint8_t stratum;            // Stratum level
  uint8_t poll;               // Poll interval
  int8_t precision;           // Precision

  // Second word (32 bits)
  uint32_t root_delay;      // Root delay
  uint32_t root_dispersion; // Root dispersion
  uint32_t reference_id;    // Reference identifier

  // Third word (32 bits) - Reference timestamp
  NtpTimestamp reference_ts;

  // Fourth word (32 bits) - Originate timestamp
  NtpTimestamp originate_ts;

  // Fifth word (32 bits) - Receive timestamp
  NtpTimestamp receive_ts;

  // Sixth word (32 bits) - Transmit timestamp
  NtpTimestamp transmit_ts;

  NtpPacket();

  /**
   * @brief Create client request packet
   * @return Client request packet
   */
  static NtpPacket createClientRequest();

  /**
   * @brief Create server response packet
   * @param client_packet Original client packet
   * @param stratum Server stratum
   * @param reference_id Reference identifier
   * @return Server response packet
   */
  static NtpPacket createServerResponse(const NtpPacket &client_packet,
                                        NtpStratum stratum,
                                        const std::string &reference_id);

  /**
   * @brief Parse from raw data
   * @param data Raw packet data
   * @return true if successful, false otherwise
   */
  bool parseFromData(const std::vector<uint8_t> &data);

  /**
   * @brief Serialize to raw data
   * @return Raw packet data
   */
  std::vector<uint8_t> serializeToData() const;

  /**
   * @brief Validate packet
   * @return true if valid, false otherwise
   */
  bool isValid() const;

  /**
   * @brief Get packet type description
   * @return String description of packet type
   */
  std::string getTypeDescription() const;

  /**
   * @brief Get packet summary for logging
   * @return Summary string
   */
  std::string getSummary() const;
};

/**
 * @brief NTP packet handler
 *
 * Handles parsing, validation, and creation of NTP packets
 */
class NtpPacketHandler {
public:
  /**
   * @brief Constructor
   */
  NtpPacketHandler();

  /**
   * @brief Destructor
   */
  ~NtpPacketHandler() = default;

  /**
   * @brief Parse incoming packet
   * @param data Raw packet data
   * @param packet Output parsed packet
   * @return true if successful, false otherwise
   */
  bool parsePacket(const std::vector<uint8_t> &data, NtpPacket &packet);

  /**
   * @brief Create response packet
   * @param request_packet Original request packet
   * @param config Server configuration
   * @return Response packet
   */
  NtpPacket createResponsePacket(const NtpPacket &request_packet,
                                 const std::shared_ptr<NtpConfig> &config);

  /**
   * @brief Validate packet format
   * @param packet Packet to validate
   * @return true if valid, false otherwise
   */
  bool validatePacket(const NtpPacket &packet) const;

  /**
   * @brief Check if packet is a client request
   * @param packet Packet to check
   * @return true if client request, false otherwise
   */
  bool isClientRequest(const NtpPacket &packet) const;

  /**
   * @brief Check if packet is a server response
   * @param packet Packet to check
   * @return true if server response, false otherwise
   */
  bool isServerResponse(const NtpPacket &packet) const;

private:
  /**
   * @brief Convert host byte order to network byte order
   * @param value Value to convert
   * @return Converted value
   */
  template <typename T> T hton(T value) const;

  /**
   * @brief Convert network byte order to host byte order
   * @param value Value to convert
   * @return Converted value
   */
  template <typename T> T ntoh(T value) const;

  /**
   * @brief Calculate round trip delay
   * @param t1 Originate timestamp
   * @param t2 Receive timestamp
   * @param t3 Transmit timestamp
   * @param t4 Response timestamp
   * @return Round trip delay in milliseconds
   */
  std::chrono::milliseconds
  calculateRoundTripDelay(const NtpTimestamp &t1, const NtpTimestamp &t2,
                          const NtpTimestamp &t3, const NtpTimestamp &t4) const;

  /**
   * @brief Calculate offset
   * @param t1 Originate timestamp
   * @param t2 Receive timestamp
   * @param t3 Transmit timestamp
   * @param t4 Response timestamp
   * @return Offset in milliseconds
   */
  std::chrono::milliseconds calculateOffset(const NtpTimestamp &t1,
                                            const NtpTimestamp &t2,
                                            const NtpTimestamp &t3,
                                            const NtpTimestamp &t4) const;
};

} // namespace simple_ntpd
