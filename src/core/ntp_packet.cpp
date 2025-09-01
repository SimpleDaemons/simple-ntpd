#include "simple_ntpd/ntp_packet.hpp"
#include "simple_ntpd/logger.hpp"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <netinet/in.h>
#include <sstream>
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#elif defined(__linux__)
#include <endian.h>
#else
#define htobe64(x) htonll(x)
#define be64toh(x) ntohll(x)
#endif

namespace simple_ntpd {

// NtpPacket implementation
NtpPacket::NtpPacket() {
  // Initialize all fields to zero/default values
  leap_indicator = 0;
  version = NTP_VERSION;
  mode = 0;
  stratum = 0;
  poll = 0;
  precision = 0;
  root_delay = 0;
  root_dispersion = 0;
  reference_id = 0;

  // Initialize timestamps
  reference_ts = NtpTimestamp();
  originate_ts = NtpTimestamp();
  receive_ts = NtpTimestamp();
  transmit_ts = NtpTimestamp();
}

NtpPacket NtpPacket::createClientRequest() {
  NtpPacket packet;
  packet.leap_indicator = 0;
  packet.version = NTP_VERSION;
  packet.mode = static_cast<uint8_t>(NtpMode::CLIENT);
  packet.stratum = 0;
  packet.poll = 4;
  packet.precision = -6;

  // Set transmit timestamp to current time
  auto now = std::chrono::system_clock::now();
  packet.transmit_ts = NtpTimestamp::fromSystemTime(now);

  return packet;
}

NtpPacket NtpPacket::createServerResponse(const NtpPacket &client_packet,
                                          NtpStratum stratum,
                                          const std::string &reference_id) {
  NtpPacket packet;
  packet.leap_indicator = 0;
  packet.version = NTP_VERSION;
  packet.mode = static_cast<uint8_t>(NtpMode::SERVER);
  packet.stratum = static_cast<uint8_t>(stratum);
  packet.poll = 4;
  packet.precision = -6;

  // Copy timestamps from client packet
  packet.originate_ts = client_packet.transmit_ts;
  packet.receive_ts =
      NtpTimestamp::fromSystemTime(std::chrono::system_clock::now());
  packet.transmit_ts =
      NtpTimestamp::fromSystemTime(std::chrono::system_clock::now());

  // Set reference timestamp and ID
  packet.reference_ts =
      NtpTimestamp::fromSystemTime(std::chrono::system_clock::now());

  // Convert reference_id string to uint32_t (first 4 characters)
  uint32_t ref_id = 0;
  for (size_t i = 0; i < std::min(reference_id.length(), size_t(4)); ++i) {
    ref_id = (ref_id << 8) | static_cast<uint8_t>(reference_id[i]);
  }
  packet.reference_id = ref_id;

  return packet;
}

bool NtpPacket::parseFromData(const std::vector<uint8_t> &data) {
  if (data.size() < NTP_PACKET_SIZE) {
    return false;
  }

  // Copy data to packet structure
  std::memcpy(this, data.data(), NTP_PACKET_SIZE);

  // Convert from network byte order
  root_delay = ntohl(root_delay);
  root_dispersion = ntohl(root_dispersion);
  reference_id = ntohl(reference_id);

  // Convert timestamps from network byte order
  reference_ts.seconds = ntohl(reference_ts.seconds);
  reference_ts.fraction = ntohl(reference_ts.fraction);
  originate_ts.seconds = ntohl(originate_ts.seconds);
  originate_ts.fraction = ntohl(originate_ts.fraction);
  receive_ts.seconds = ntohl(receive_ts.seconds);
  receive_ts.fraction = ntohl(receive_ts.fraction);
  transmit_ts.seconds = ntohl(transmit_ts.seconds);
  transmit_ts.fraction = ntohl(transmit_ts.fraction);

  return true;
}

std::vector<uint8_t> NtpPacket::serializeToData() const {
  std::vector<uint8_t> data(NTP_PACKET_SIZE);

  // Create a copy for serialization
  NtpPacket packet_copy = *this;

  // Convert to network byte order
  packet_copy.root_delay = htonl(packet_copy.root_delay);
  packet_copy.root_dispersion = htonl(packet_copy.root_dispersion);
  packet_copy.reference_id = htonl(packet_copy.reference_id);

  // Convert timestamps to network byte order
  packet_copy.reference_ts.seconds = htonl(packet_copy.reference_ts.seconds);
  packet_copy.reference_ts.fraction = htonl(packet_copy.reference_ts.fraction);
  packet_copy.originate_ts.seconds = htonl(packet_copy.originate_ts.seconds);
  packet_copy.originate_ts.fraction = htonl(packet_copy.originate_ts.fraction);
  packet_copy.receive_ts.seconds = htonl(packet_copy.receive_ts.seconds);
  packet_copy.receive_ts.fraction = htonl(packet_copy.receive_ts.fraction);
  packet_copy.transmit_ts.seconds = htonl(packet_copy.transmit_ts.seconds);
  packet_copy.transmit_ts.fraction = htonl(packet_copy.transmit_ts.fraction);

  std::memcpy(data.data(), &packet_copy, NTP_PACKET_SIZE);
  return data;
}

bool NtpPacket::isValid() const {
  // Check version
  if (version != NTP_VERSION) {
    return false;
  }

  // Check mode
  if (mode != static_cast<uint8_t>(NtpMode::CLIENT) &&
      mode != static_cast<uint8_t>(NtpMode::SERVER) &&
      mode != static_cast<uint8_t>(NtpMode::BROADCAST)) {
    return false;
  }

  // Check stratum
  if (stratum > 15) {
    return false;
  }

  // Check poll interval
  if (poll < 4 || poll > 17) {
    return false;
  }

  return true;
}

std::string NtpPacket::getTypeDescription() const {
  std::stringstream ss;
  ss << "NTP v" << static_cast<int>(version) << " ";

  switch (static_cast<NtpMode>(mode)) {
  case NtpMode::CLIENT:
    ss << "Client";
    break;
  case NtpMode::SERVER:
    ss << "Server";
    break;
  case NtpMode::BROADCAST:
    ss << "Broadcast";
    break;
  default:
    ss << "Unknown";
    break;
  }

  ss << " (Stratum " << static_cast<int>(stratum) << ")";
  return ss.str();
}

std::string NtpPacket::getSummary() const {
  std::stringstream ss;
  ss << "NTP Packet: " << getTypeDescription() << "\n";
  ss << "  Leap: " << static_cast<int>(leap_indicator) << "\n";
  ss << "  Poll: " << static_cast<int>(poll) << "\n";
  ss << "  Precision: " << static_cast<int>(precision) << "\n";
  ss << "  Root Delay: " << root_delay << "\n";
  ss << "  Root Dispersion: " << root_dispersion << "\n";
  ss << "  Reference ID: " << std::hex << reference_id << std::dec;
  return ss.str();
}

// NtpPacketHandler implementation
NtpPacketHandler::NtpPacketHandler() = default;

bool NtpPacketHandler::parsePacket(const std::vector<uint8_t> &data,
                                   NtpPacket &packet) {
  return packet.parseFromData(data);
}

NtpPacket NtpPacketHandler::createResponsePacket(
    const NtpPacket &request_packet, const std::shared_ptr<NtpConfig> &config) {
  // Create server response with default values
  return NtpPacket::createServerResponse(
      request_packet, NtpStratum::SECONDARY_REFERENCE, "LOCL");
}

bool NtpPacketHandler::validatePacket(const NtpPacket &packet) const {
  return packet.isValid();
}

bool NtpPacketHandler::isClientRequest(const NtpPacket &packet) const {
  return packet.mode == static_cast<uint8_t>(NtpMode::CLIENT);
}

bool NtpPacketHandler::isServerResponse(const NtpPacket &packet) const {
  return packet.mode == static_cast<uint8_t>(NtpMode::SERVER);
}

template <typename T> T NtpPacketHandler::hton(T value) const {
  if constexpr (sizeof(T) == 2) {
    return htons(value);
  } else if constexpr (sizeof(T) == 4) {
    return htonl(value);
  } else if constexpr (sizeof(T) == 8) {
    return htobe64(value);
  }
  return value;
}

template <typename T> T NtpPacketHandler::ntoh(T value) const {
  if constexpr (sizeof(T) == 2) {
    return ntohs(value);
  } else if constexpr (sizeof(T) == 4) {
    return ntohl(value);
  } else if constexpr (sizeof(T) == 8) {
    return be64toh(value);
  }
  return value;
}

std::chrono::milliseconds NtpPacketHandler::calculateRoundTripDelay(
    const NtpTimestamp &t1, const NtpTimestamp &t2, const NtpTimestamp &t3,
    const NtpTimestamp &t4) const {

  // Convert NTP timestamps to system time
  auto time1 = t1.toSystemTime();
  auto time2 = t2.toSystemTime();
  auto time3 = t3.toSystemTime();
  auto time4 = t4.toSystemTime();

  // Calculate RTT = (t4 - t1) - (t3 - t2)
  auto delay = (time4 - time1) - (time3 - time2);
  return std::chrono::duration_cast<std::chrono::milliseconds>(delay);
}

std::chrono::milliseconds NtpPacketHandler::calculateOffset(
    const NtpTimestamp &t1, const NtpTimestamp &t2, const NtpTimestamp &t3,
    const NtpTimestamp &t4) const {

  // Convert NTP timestamps to system time
  auto time1 = t1.toSystemTime();
  auto time2 = t2.toSystemTime();
  auto time3 = t3.toSystemTime();
  auto time4 = t4.toSystemTime();

  // Calculate offset = ((t2 - t1) + (t3 - t4)) / 2
  auto offset = ((time2 - time1) + (time3 - time4)) / 2;
  return std::chrono::duration_cast<std::chrono::milliseconds>(offset);
}

// NtpTimestamp implementation
NtpTimestamp NtpTimestamp::fromSystemTime(
    const std::chrono::system_clock::time_point &time) {
  // Convert system time to NTP timestamp
  // NTP epoch starts at January 1, 1900, system_clock epoch starts at January
  // 1, 1970 Difference is 70 years + 17 leap days = 2208988800 seconds

  constexpr uint64_t NTP_EPOCH_OFFSET = 2208988800ULL;

  auto duration = time.time_since_epoch();
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
  auto fraction =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

  // Convert fraction to NTP fraction (2^32 units)
  uint32_t ntp_fraction =
      static_cast<uint32_t>((fraction.count() * 4294967296ULL) / 1000000000ULL);

  return NtpTimestamp(static_cast<uint32_t>(seconds.count() + NTP_EPOCH_OFFSET),
                      ntp_fraction);
}

std::chrono::system_clock::time_point NtpTimestamp::toSystemTime() const {
  // Convert NTP timestamp to system time
  constexpr uint64_t NTP_EPOCH_OFFSET = 2208988800ULL;

  // Convert everything to microseconds to match system_clock precision
  auto total_microseconds = std::chrono::microseconds(
      (static_cast<uint64_t>(seconds - NTP_EPOCH_OFFSET) * 1000000ULL) +
      ((static_cast<uint64_t>(fraction) * 1000000ULL) / 4294967296ULL));

  // Create time point from epoch and add the duration
  return std::chrono::system_clock::from_time_t(0) + total_microseconds;
}

NtpTimestamp NtpTimestamp::now() {
  return fromSystemTime(std::chrono::system_clock::now());
}

} // namespace simple_ntpd
