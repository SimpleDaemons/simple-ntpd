/**
 * @file upstream_sync.hpp
 * @brief Upstream NTP synchronization
 */

#pragma once

#include "simple-ntpd/config/config.hpp"
#include "simple-ntpd/utils/logger.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace simple_ntpd {

struct UpstreamSyncResult {
  bool success = false;
  std::string server;
  int64_t offset_us = 0;
  int64_t delay_us = 0;
  uint8_t stratum = 0;
};

/** Query a single upstream NTP server (UDP port 123). */
UpstreamSyncResult queryUpstreamServer(const std::string &host,
                                       std::chrono::milliseconds timeout);

/**
 * @brief Background upstream synchronization manager.
 *
 * Periodically queries configured upstream servers and tracks clock offset
 * and stratum for downstream responses.
 */
class UpstreamSyncManager {
public:
  UpstreamSyncManager(std::shared_ptr<NtpConfig> config,
                      std::shared_ptr<Logger> logger);
  ~UpstreamSyncManager();

  void start();
  void stop();

  UpstreamSyncResult syncOnce();

  int64_t clockOffsetUs() const;
  uint8_t effectiveStratum(uint8_t configured_stratum) const;
  std::string syncedUpstream() const;
  std::chrono::system_clock::time_point lastSyncTime() const;
  bool isSynced() const;
  std::string statusSummary() const;

private:
  void syncLoop();

  std::shared_ptr<NtpConfig> config_;
  std::shared_ptr<Logger> logger_;
  std::thread sync_thread_;
  std::atomic<bool> running_{false};

  mutable std::mutex state_mutex_;
  int64_t clock_offset_us_ = 0;
  int64_t last_delay_us_ = 0;
  uint8_t upstream_stratum_ = 0;
  std::string synced_upstream_;
  std::chrono::system_clock::time_point last_sync_time_{};
  bool synced_ = false;
};

} // namespace simple_ntpd
