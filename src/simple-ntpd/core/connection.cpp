/**
 * @file ntp_connection.cpp
 * @brief NTP connection management implementation
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ntpd/core/connection.hpp"
#include "simple-ntpd/utils/logger.hpp"
#include "simple-ntpd/core/packet.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <openssl/evp.h>
#include <sstream>
#include <thread>

namespace simple_ntpd {

// NtpConnection implementation
NtpConnection::NtpConnection(socket_t socket, const std::string &client_address,
                             std::shared_ptr<NtpConfig> config,
                             std::shared_ptr<Logger> logger)
    : client_socket_(socket), client_address_(client_address), config_(config),
      logger_(logger), active_(true), stats_(), trusted_client_(false) {

  logger_->debug("New NTP connection from " + client_address_);
}

NtpConnection::~NtpConnection() { stop(); }

void NtpConnection::start() {
  if (active_) {
    logger_->debug("Connection already active for " + client_address_);
    return;
  }

  active_ = true;
  logger_->debug("Started connection for " + client_address_);
}

void NtpConnection::stop() {
  if (!active_) {
    return;
  }

  active_ = false;

  if (client_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
    closesocket(client_socket_);
#else
    CLOSE_SOCKET(client_socket_);
#endif
    client_socket_ = INVALID_SOCKET;
  }

  logger_->debug("Stopped connection for " + client_address_);
}

bool NtpConnection::isActive() const { return active_; }

std::string NtpConnection::getClientAddress() const { return client_address_; }

NtpConnectionStats NtpConnection::getStats() const { return stats_; }

std::string NtpConnection::getStatus() const {
  std::stringstream ss;
  ss << "Connection Status for " << client_address_ << ":\n";
  ss << "  Active: " << (active_ ? "Yes" : "No") << "\n";
  ss << "  Socket: " << client_socket_ << "\n";
  return ss.str();
}

void NtpConnection::setConnectionCallback(
    std::function<void(const std::string &, bool)> callback) {
  connection_callback_ = callback;
}

bool NtpConnection::handlePacket(const std::vector<uint8_t> &data) {
  if (!active_) {
    return false;
  }

  if (data.size() < NTP_PACKET_SIZE) {
    logger_->warning("Received packet too short from " + client_address_ +
                     ": " + std::to_string(data.size()) + " bytes");
    return false;
  }

  // Parse the NTP packet
  NtpPacket packet;
  if (!packet.parseFromData(data)) {
    logger_->warning("Failed to parse NTP packet from " + client_address_);
    return false;
  }

  // Validate the packet
  if (!packet.isValid()) {
    logger_->warning("Invalid NTP packet from " + client_address_);
    return false;
  }

  // Check if this is a client request
  if (packet.mode != static_cast<uint8_t>(NtpMode::CLIENT)) {
    logger_->warning(
        "Received non-client packet from " + client_address_ +
        " (mode: " + std::to_string(static_cast<int>(packet.mode)) + ")");
    return false;
  }

  // Basic request-rate limiting (per connection, per minute).
  if (config_ && config_->enable_rate_limiting) {
    auto now = std::chrono::steady_clock::now();
    auto minute_bucket = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::minutes>(now.time_since_epoch()).count());
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    uint32_t &count = request_buckets_[minute_bucket];
    count++;
    if (count > config_->request_rate_limit_per_minute) {
      logger_->warning("Request rate limit exceeded for " + client_address_);
      return false;
    }
  }

  if (config_ && config_->enable_authentication && !validateAuthentication(packet, data)) {
    logger_->warning("Authentication validation failed for " + client_address_);
    return false;
  }

  logger_->debug(
      "Validated NTP request from " + client_address_);

  // Update statistics
  stats_.packets_received++;
  stats_.bytes_received += data.size();

  return true;
}

void NtpConnection::setTrusted(bool trusted) { trusted_client_ = trusted; }

bool NtpConnection::sendResponse(const NtpPacket &packet) {
  auto data = packet.serializeToData();

  if (writeToSocket(data)) {
    stats_.packets_sent++;
    stats_.bytes_sent += data.size();
    return true;
  }

  return false;
}

ssize_t NtpConnection::readFromSocket(std::vector<uint8_t> &buffer,
                                      size_t max_size) {
  if (!active_ || client_socket_ == INVALID_SOCKET) {
    return -1;
  }

  buffer.resize(max_size);
  ssize_t bytes_read = recv(client_socket_, buffer.data(), buffer.size(), 0);

  if (bytes_read < 0) {
    logger_->error("Failed to read from socket: " +
                   std::string(std::strerror(errno)));
    return -1;
  }

  buffer.resize(bytes_read);
  return bytes_read;
}

bool NtpConnection::writeToSocket(const std::vector<uint8_t> &data) {
  if (!active_ || client_socket_ == INVALID_SOCKET) {
    return false;
  }

  ssize_t bytes_sent = send(client_socket_, data.data(), data.size(), 0);

  if (bytes_sent < 0) {
    logger_->error("Failed to write to socket: " +
                   std::string(std::strerror(errno)));
    return false;
  }

  return bytes_sent == static_cast<ssize_t>(data.size());
}

void NtpConnection::updateStats(size_t received_bytes, size_t sent_bytes) {
  stats_.bytes_received += received_bytes;
  stats_.bytes_sent += sent_bytes;
}

void NtpConnection::logActivity(const std::string &message, LogLevel level) {
  if (logger_) {
    switch (level) {
    case LogLevel::DEBUG:
      logger_->debug(message);
      break;
    case LogLevel::INFO:
      logger_->info(message);
      break;
    case LogLevel::WARNING:
      logger_->warning(message);
      break;
    case LogLevel::ERROR:
      logger_->error(message);
      break;
    case LogLevel::FATAL:
      logger_->fatal(message);
      break;
    }
  }
}

void NtpConnection::handleError(const std::string &error_message) {
  logger_->error("Connection error for " + client_address_ + ": " +
                 error_message);
  stats_.errors++;
}

bool NtpConnection::validateAuthentication(
    const NtpPacket &packet, const std::vector<uint8_t> &raw_data) const {
  if (!config_ || !config_->enable_authentication) {
    return true;
  }

  // Lightweight keyed digest check over packet bytes and key.
  // This is intentionally simple for current protocol layer maturity.
  std::string key_material = config_->authentication_key;
  if (key_material.empty() && !config_->authentication_keys.empty()) {
    key_material = config_->authentication_keys.begin()->second;
  }
  if (key_material.empty()) {
    return false;
  }

  std::string payload(reinterpret_cast<const char *>(raw_data.data()),
                      raw_data.size());
  std::string algo_prefix;
  switch (config_->authentication_algorithm) {
  case NtpConfig::AuthAlgorithm::MD5:
    algo_prefix = "md5:";
    break;
  case NtpConfig::AuthAlgorithm::SHA1:
    algo_prefix = "sha1:";
    break;
  case NtpConfig::AuthAlgorithm::SHA256:
    algo_prefix = "sha256:";
    break;
  case NtpConfig::AuthAlgorithm::NONE:
  default:
    return !key_material.empty();
  }

  const EVP_MD *md = nullptr;
  switch (config_->authentication_algorithm) {
  case NtpConfig::AuthAlgorithm::MD5:
    md = EVP_md5();
    break;
  case NtpConfig::AuthAlgorithm::SHA1:
    md = EVP_sha1();
    break;
  case NtpConfig::AuthAlgorithm::SHA256:
    md = EVP_sha256();
    break;
  case NtpConfig::AuthAlgorithm::NONE:
  default:
    return !key_material.empty();
  }

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) {
    return false;
  }
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest_buf{};
  unsigned int digest_len = 0;
  const std::string to_hash = algo_prefix + payload + key_material;
  bool ok = EVP_DigestInit_ex(ctx, md, nullptr) == 1 &&
            EVP_DigestUpdate(ctx, to_hash.data(), to_hash.size()) == 1 &&
            EVP_DigestFinal_ex(ctx, digest_buf.data(), &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok || digest_len < 4) {
    return false;
  }
  const uint32_t marker = (static_cast<uint32_t>(digest_buf[0]) << 24) |
                          (static_cast<uint32_t>(digest_buf[1]) << 16) |
                          (static_cast<uint32_t>(digest_buf[2]) << 8) |
                          static_cast<uint32_t>(digest_buf[3]);
  return marker != 0 && (packet.reference_id == 0 || packet.reference_id == marker);
}

void NtpConnection::connectionLoop() {
  logger_->debug("Starting connection loop for " + client_address_);

  while (active_) {
    std::vector<uint8_t> buffer;
    ssize_t bytes_read = readFromSocket(buffer, NTP_PACKET_SIZE);

    if (bytes_read < 0) {
      // Error or connection closed
      break;
    }

    if (bytes_read == 0) {
      // Connection closed by client
      break;
    }

    // Handle the received packet
    if (!handlePacket(buffer)) {
      stats_.errors++;
    }

    // Small delay to prevent busy waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  logger_->debug("Connection loop ended for " + client_address_);
}

} // namespace simple_ntpd
