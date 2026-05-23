/**
 * @file upstream_sync.cpp
 * @brief Upstream NTP synchronization
 */

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "simple-ntpd/core/upstream_sync.hpp"
#include "simple-ntpd/core/packet.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>

namespace simple_ntpd {

namespace {

socket_t createUdpSocket(std::chrono::milliseconds timeout) {
  socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET) {
    return INVALID_SOCKET;
  }

  const auto ms = static_cast<int>(timeout.count());
  struct timeval tv {};
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  return sock;
}

} // namespace

UpstreamSyncResult queryUpstreamServer(const std::string &host,
                                       std::chrono::milliseconds timeout) {
  UpstreamSyncResult result;
  result.server = host;

  struct addrinfo hints {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *res = nullptr;
  const int gai = getaddrinfo(host.c_str(), "123", &hints, &res);
  if (gai != 0 || res == nullptr) {
    return result;
  }

  socket_t sock = createUdpSocket(timeout);
  if (sock == INVALID_SOCKET) {
    freeaddrinfo(res);
    return result;
  }

  NtpPacket request = NtpPacket::createClientRequest();
  const auto request_data = request.serializeToData();
  const NtpTimestamp t1 = request.transmit_ts;

  bool sent_any = false;
  for (struct addrinfo *ai = res; ai != nullptr; ai = ai->ai_next) {
    if (ai->ai_family != AF_INET) {
      continue;
    }
    ssize_t sent = sendto(sock, request_data.data(), request_data.size(), 0,
                          ai->ai_addr, ai->ai_addrlen);
    if (sent == static_cast<ssize_t>(request_data.size())) {
      sent_any = true;
      break;
    }
  }
  freeaddrinfo(res);
  if (!sent_any) {
    CLOSE_SOCKET(sock);
    return result;
  }

  std::array<uint8_t, NTP_PACKET_SIZE> buffer{};
  struct sockaddr_in from_addr {};
  socklen_t from_len = sizeof(from_addr);
  const ssize_t received =
      recvfrom(sock, buffer.data(), buffer.size(), 0,
               reinterpret_cast<struct sockaddr *>(&from_addr), &from_len);
  CLOSE_SOCKET(sock);

  if (received < static_cast<ssize_t>(NTP_PACKET_SIZE)) {
    return result;
  }

  NtpPacket response;
  std::vector<uint8_t> response_data(buffer.begin(), buffer.begin() + received);
  if (!response.parseFromData(response_data) || !response.isValid()) {
    return result;
  }

  if (response.mode != static_cast<uint8_t>(NtpMode::SERVER)) {
    return result;
  }

  const NtpTimestamp t4 = NtpTimestamp::now();
  NtpPacketHandler handler;
  result.offset_us =
      handler.calculateOffset(t1, response.receive_ts, response.transmit_ts, t4)
          .count();
  result.delay_us = handler
                        .calculateRoundTripDelay(t1, response.receive_ts,
                                                 response.transmit_ts, t4)
                        .count();
  result.stratum = response.stratum;
  result.success = true;
  return result;
}

UpstreamSyncManager::UpstreamSyncManager(std::shared_ptr<NtpConfig> config,
                                         std::shared_ptr<Logger> logger)
    : config_(std::move(config)), logger_(std::move(logger)) {}

UpstreamSyncManager::~UpstreamSyncManager() { stop(); }

void UpstreamSyncManager::start() {
  if (running_.exchange(true)) {
    return;
  }
  sync_thread_ = std::thread(&UpstreamSyncManager::syncLoop, this);
}

void UpstreamSyncManager::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  if (sync_thread_.joinable()) {
    sync_thread_.join();
  }
}

UpstreamSyncResult UpstreamSyncManager::syncOnce() {
  if (!config_ || config_->upstream_servers.empty()) {
    return UpstreamSyncResult{};
  }

  UpstreamSyncResult best;

  // Try all configured servers; keep the result with lowest delay.
  for (const auto &server : config_->upstream_servers) {
    UpstreamSyncResult attempt =
        queryUpstreamServer(server, config_->timeout);
    if (attempt.success) {
      if (!best.success || attempt.delay_us < best.delay_us) {
        best = attempt;
      }
    } else if (logger_) {
      logger_->debug("Upstream sync failed for " + server);
    }
  }

  if (best.success) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    clock_offset_us_ = best.offset_us;
    last_delay_us_ = best.delay_us;
    upstream_stratum_ = best.stratum;
    synced_upstream_ = best.server;
    last_sync_time_ = std::chrono::system_clock::now();
    synced_ = true;
    if (logger_) {
      logger_->info("Synced with upstream " + best.server + " offset_us=" +
                    std::to_string(best.offset_us) + " stratum=" +
                    std::to_string(best.stratum));
    }
  } else if (logger_) {
    logger_->warning("Upstream synchronization failed for all configured servers");
  }

  return best;
}

void UpstreamSyncManager::syncLoop() {
  // Initial sync soon after startup.
  syncOnce();

  while (running_) {
    const auto interval =
        config_ ? config_->sync_interval : std::chrono::seconds(64);
  std::this_thread::sleep_for(interval);
    if (!running_) {
      break;
    }
    syncOnce();
  }
}

int64_t UpstreamSyncManager::clockOffsetUs() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return clock_offset_us_;
}

uint8_t UpstreamSyncManager::effectiveStratum(uint8_t configured_stratum) const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (!synced_ || upstream_stratum_ == 0) {
    return configured_stratum;
  }
  const int derived = static_cast<int>(upstream_stratum_) + 1;
  return static_cast<uint8_t>(std::clamp(derived, 1, 15));
}

std::string UpstreamSyncManager::syncedUpstream() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return synced_upstream_;
}

std::chrono::system_clock::time_point UpstreamSyncManager::lastSyncTime() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return last_sync_time_;
}

bool UpstreamSyncManager::isSynced() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return synced_;
}

std::string UpstreamSyncManager::statusSummary() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  std::stringstream ss;
  ss << "  Upstream Sync: " << (synced_ ? "Synced" : "Not synced") << "\n";
  if (synced_) {
    ss << "  Synced Upstream: " << synced_upstream_ << "\n";
    ss << "  Clock Offset (us): " << clock_offset_us_ << "\n";
    ss << "  Last RTT (us): " << last_delay_us_ << "\n";
    ss << "  Upstream Stratum: " << static_cast<int>(upstream_stratum_) << "\n";
  }
  return ss.str();
}

} // namespace simple_ntpd
