#include "simple_ntpd/ntp_packet.hpp"
#include "simple_ntpd/logger.hpp"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace simple_ntpd {

// NtpTimestamp implementation
NtpTimestamp::NtpTimestamp() : seconds_(0), fraction_(0) {}

NtpTimestamp::NtpTimestamp(uint32_t seconds, uint32_t fraction) 
    : seconds_(seconds), fraction_(fraction) {}

NtpTimestamp::NtpTimestamp(const std::chrono::system_clock::time_point& time_point) {
    auto duration = time_point.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto fraction_duration = duration - seconds;
    
    // Convert to NTP epoch (1900-01-01 00:00:00 UTC)
    const uint32_t NTP_EPOCH_OFFSET = 2208988800UL;
    seconds_ = static_cast<uint32_t>(seconds.count()) + NTP_EPOCH_OFFSET;
    
    // Convert fraction to NTP fraction (2^-32 precision)
    auto fraction_seconds = std::chrono::duration_cast<std::chrono::nanoseconds>(fraction_duration);
    fraction_ = static_cast<uint32_t>((fraction_seconds.count() * 4294967296ULL) / 1000000000ULL);
}

std::chrono::system_clock::time_point NtpTimestamp::toTimePoint() const {
    const uint32_t NTP_EPOCH_OFFSET = 2208988800UL;
    uint64_t total_seconds = static_cast<uint64_t>(seconds_) - NTP_EPOCH_OFFSET;
    
    auto duration = std::chrono::seconds(total_seconds);
    auto fraction_duration = std::chrono::nanoseconds(
        static_cast<int64_t>((static_cast<uint64_t>(fraction_) * 1000000000ULL) / 4294967296ULL)
    );
    
    return std::chrono::system_clock::time_point(duration + fraction_duration);
}

uint32_t NtpTimestamp::getSeconds() const { return seconds_; }
uint32_t NtpTimestamp::getFraction() const { return fraction_; }
void NtpTimestamp::setSeconds(uint32_t seconds) { seconds_ = seconds; }
void NtpTimestamp::setFraction(uint32_t fraction) { fraction_ = fraction; }

std::string NtpTimestamp::toString() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << seconds_;
    ss << "." << std::hex << std::setfill('0') << std::setw(8) << fraction_;
    return ss.str();
}

// NtpPacket implementation
NtpPacket::NtpPacket() { clear(); }

void NtpPacket::clear() {
    std::memset(&packet_, 0, sizeof(packet_));
    
    packet_.li_vn_mode = (NtpLeapIndicator::NO_WARNING << 6) | 
                         (NTP_VERSION << 3) | 
                         (NtpMode::CLIENT << 0);
    packet_.stratum = NtpStratum::UNSYNC;
    packet_.poll = 4;
    packet_.precision = static_cast<int8_t>(-6);
    packet_.root_delay = 0;
    packet_.root_dispersion = 0;
    packet_.reference_id = 0;
}

bool NtpPacket::parse(const uint8_t* data, size_t length) {
    if (length < sizeof(NtpPacketData)) {
        Logger::getInstance().error("NTP packet too short: " + std::to_string(length) + " bytes");
        return false;
    }
    
    std::memcpy(&packet_, data, sizeof(NtpPacketData));
    
    // Convert from network byte order
    packet_.root_delay = ntohl(packet_.root_delay);
    packet_.root_dispersion = ntohl(packet_.root_dispersion);
    packet_.reference_id = ntohl(packet_.reference_id);
    
    // Convert timestamps
    packet_.reference_ts.setSeconds(ntohl(packet_.reference_ts.getSeconds()));
    packet_.reference_ts.setFraction(ntohl(packet_.reference_ts.getFraction()));
    packet_.origin_ts.setSeconds(ntohl(packet_.origin_ts.getSeconds()));
    packet_.origin_ts.setFraction(ntohl(packet_.origin_ts.getFraction()));
    packet_.receive_ts.setSeconds(ntohl(packet_.receive_ts.getSeconds()));
    packet_.receive_ts.setFraction(ntohl(packet_.receive_ts.getFraction()));
    packet_.transmit_ts.setSeconds(ntohl(packet_.transmit_ts.getSeconds()));
    packet_.transmit_ts.setFraction(ntohl(packet_.transmit_ts.getFraction()));
    
    return true;
}

std::vector<uint8_t> NtpPacket::serialize() const {
    std::vector<uint8_t> data(sizeof(NtpPacketData));
    
    NtpPacketData packet_copy = packet_;
    
    // Convert to network byte order
    packet_copy.root_delay = htonl(packet_copy.root_delay);
    packet_copy.root_dispersion = htonl(packet_copy.root_dispersion);
    packet_copy.reference_id = htonl(packet_copy.reference_id);
    
    // Convert timestamps
    packet_copy.reference_ts.setSeconds(htonl(packet_copy.reference_ts.getSeconds()));
    packet_copy.reference_ts.setFraction(htonl(packet_copy.reference_ts.getFraction()));
    packet_copy.origin_ts.setSeconds(htonl(packet_copy.origin_ts.getSeconds()));
    packet_copy.origin_ts.setFraction(htonl(packet_copy.origin_ts.getFraction()));
    packet_copy.receive_ts.setSeconds(htonl(packet_copy.receive_ts.getSeconds()));
    packet_copy.receive_ts.setFraction(htonl(packet_copy.receive_ts.getFraction()));
    packet_copy.transmit_ts.setSeconds(htonl(packet_copy.transmit_ts.getSeconds()));
    packet_copy.transmit_ts.setFraction(htonl(packet_copy.transmit_ts.getFraction()));
    
    std::memcpy(data.data(), &packet_copy, sizeof(NtpPacketData));
    return data;
}

bool NtpPacket::isValid() const {
    uint8_t version = (packet_.li_vn_mode >> 3) & 0x07;
    if (version != NTP_VERSION) return false;
    
    uint8_t mode = packet_.li_vn_mode & 0x07;
    if (mode != static_cast<uint8_t>(NtpMode::CLIENT) && 
        mode != static_cast<uint8_t>(NtpMode::SERVER) &&
        mode != static_cast<uint8_t>(NtpMode::BROADCAST)) return false;
    
    if (packet_.stratum < 0 || packet_.stratum > 15) return false;
    if (packet_.poll < 4 || packet_.poll > 17) return false;
    if (packet_.precision < -128 || packet_.precision > 127) return false;
    
    return true;
}

// Getters and setters
NtpLeapIndicator NtpPacket::getLeapIndicator() const {
    return static_cast<NtpLeapIndicator>((packet_.li_vn_mode >> 6) & 0x03);
}

void NtpPacket::setLeapIndicator(NtpLeapIndicator li) {
    packet_.li_vn_mode = (packet_.li_vn_mode & 0x3F) | (static_cast<uint8_t>(li) << 6);
}

uint8_t NtpPacket::getVersion() const {
    return (packet_.li_vn_mode >> 3) & 0x07;
}

void NtpPacket::setVersion(uint8_t version) {
    packet_.li_vn_mode = (packet_.li_vn_mode & 0xC7) | ((version & 0x07) << 3);
}

NtpMode NtpPacket::getMode() const {
    return static_cast<NtpMode>(packet_.li_vn_mode & 0x07);
}

void NtpPacket::setMode(NtpMode mode) {
    packet_.li_vn_mode = (packet_.li_vn_mode & 0xF8) | static_cast<uint8_t>(mode);
}

uint8_t NtpPacket::getStratum() const { return packet_.stratum; }
void NtpPacket::setStratum(uint8_t stratum) { packet_.stratum = stratum; }
int8_t NtpPacket::getPoll() const { return packet_.poll; }
void NtpPacket::setPoll(int8_t poll) { packet_.poll = poll; }
int8_t NtpPacket::getPrecision() const { return packet_.precision; }
void NtpPacket::setPrecision(int8_t precision) { packet_.precision = precision; }

uint32_t NtpPacket::getRootDelay() const { return packet_.root_delay; }
void NtpPacket::setRootDelay(uint32_t delay) { packet_.root_delay = delay; }
uint32_t NtpPacket::getRootDispersion() const { return packet_.root_dispersion; }
void NtpPacket::setRootDispersion(uint32_t dispersion) { packet_.root_dispersion = dispersion; }
uint32_t NtpPacket::getReferenceId() const { return packet_.reference_id; }
void NtpPacket::setReferenceId(uint32_t id) { packet_.reference_id = id; }

NtpTimestamp NtpPacket::getReferenceTimestamp() const { return packet_.reference_ts; }
void NtpPacket::setReferenceTimestamp(const NtpTimestamp& ts) { packet_.reference_ts = ts; }
NtpTimestamp NtpPacket::getOriginTimestamp() const { return packet_.origin_ts; }
void NtpPacket::setOriginTimestamp(const NtpTimestamp& ts) { packet_.origin_ts = ts; }
NtpTimestamp NtpPacket::getReceiveTimestamp() const { return packet_.receive_ts; }
void NtpPacket::setReceiveTimestamp(const NtpTimestamp& ts) { packet_.receive_ts = ts; }
NtpTimestamp NtpPacket::getTransmitTimestamp() const { return packet_.transmit_ts; }
void NtpPacket::setTransmitTimestamp(const NtpTimestamp& ts) { packet_.transmit_ts = ts; }

std::string NtpPacket::toString() const {
    std::stringstream ss;
    ss << "NTP Packet:\n";
    ss << "  Leap Indicator: " << static_cast<int>(getLeapIndicator()) << "\n";
    ss << "  Version: " << static_cast<int>(getVersion()) << "\n";
    ss << "  Mode: " << static_cast<int>(getMode()) << "\n";
    ss << "  Stratum: " << static_cast<int>(getStratum()) << "\n";
    ss << "  Poll: " << static_cast<int>(getPoll()) << "\n";
    ss << "  Precision: " << static_cast<int>(getPrecision()) << "\n";
    ss << "  Root Delay: " << getRootDelay() << "\n";
    ss << "  Root Dispersion: " << getRootDispersion() << "\n";
    ss << "  Reference ID: " << std::hex << getReferenceId() << std::dec << "\n";
    ss << "  Reference Timestamp: " << getReferenceTimestamp().toString() << "\n";
    ss << "  Origin Timestamp: " << getOriginTimestamp().toString() << "\n";
    ss << "  Receive Timestamp: " << getReceiveTimestamp().toString() << "\n";
    ss << "  Transmit Timestamp: " << getTransmitTimestamp().toString() << "\n";
    return ss.str();
}

// NtpPacketHandler implementation
std::unique_ptr<NtpPacket> NtpPacketHandler::createClientRequest() {
    auto packet = std::make_unique<NtpPacket>();
    
    packet->setMode(NtpMode::CLIENT);
    packet->setVersion(NTP_VERSION);
    packet->setLeapIndicator(NtpLeapIndicator::NO_WARNING);
    packet->setStratum(0);
    packet->setPoll(4);
    packet->setPrecision(-6);
    packet->setRootDelay(0);
    packet->setRootDispersion(0);
    packet->setReferenceId(0);
    
    packet->setTransmitTimestamp(NtpTimestamp(std::chrono::system_clock::now()));
    
    return packet;
}

std::unique_ptr<NtpPacket> NtpPacketHandler::createServerResponse(const NtpPacket& request, 
                                                                 uint8_t stratum,
                                                                 const NtpTimestamp& reference_ts,
                                                                 uint32_t reference_id) {
    auto response = std::make_unique<NtpPacket>();
    
    response->setMode(NtpMode::SERVER);
    response->setVersion(NTP_VERSION);
    response->setLeapIndicator(NtpLeapIndicator::NO_WARNING);
    response->setStratum(stratum);
    response->setPoll(4);
    response->setPrecision(-6);
    response->setRootDelay(0);
    response->setRootDispersion(0);
    response->setReferenceId(reference_id);
    response->setReferenceTimestamp(reference_ts);
    
    response->setOriginTimestamp(request.getTransmitTimestamp());
    response->setReceiveTimestamp(NtpTimestamp(std::chrono::system_clock::now()));
    response->setTransmitTimestamp(NtpTimestamp(std::chrono::system_clock::now()));
    
    return response;
}

bool NtpPacketHandler::validatePacket(const NtpPacket& packet) {
    return packet.isValid();
}

std::chrono::microseconds NtpPacketHandler::calculateRoundTripDelay(const NtpPacket& request, 
                                                                   const NtpPacket& response) {
    auto t1 = request.getTransmitTimestamp().toTimePoint();
    auto t2 = response.getReceiveTimestamp().toTimePoint();
    auto t3 = response.getTransmitTimestamp().toTimePoint();
    auto t4 = std::chrono::system_clock::now();
    
    auto delay = (t4 - t1) - (t3 - t2);
    return std::chrono::duration_cast<std::chrono::microseconds>(delay);
}

std::chrono::microseconds NtpPacketHandler::calculateOffset(const NtpPacket& request, 
                                                           const NtpPacket& response) {
    auto t1 = request.getTransmitTimestamp().toTimePoint();
    auto t2 = response.getReceiveTimestamp().toTimePoint();
    auto t3 = response.getTransmitTimestamp().toTimePoint();
    auto t4 = std::chrono::system_clock::now();
    
    auto offset = ((t2 - t1) + (t3 - t4)) / 2;
    return std::chrono::duration_cast<std::chrono::microseconds>(offset);
}

} // namespace simple_ntpd

