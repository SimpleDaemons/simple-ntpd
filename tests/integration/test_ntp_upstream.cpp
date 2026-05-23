/**
 * @file test_ntp_upstream.cpp
 * @brief Upstream NTP synchronization tests
 */

#include "simple-ntpd/core/upstream_sync.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace simple_ntpd;

int main() {
  std::cout << "Running NTP Upstream Sync Tests..." << std::endl;

  if (std::getenv("SIMPLE_NTPD_NETWORK_TESTS") == nullptr) {
    std::cout << "Skipping live upstream tests (set SIMPLE_NTPD_NETWORK_TESTS=1 to enable)."
              << std::endl;

    auto config = std::make_shared<NtpConfig>();
    UpstreamSyncManager manager(config,
                                std::shared_ptr<Logger>(&Logger::getInstance(),
                                                        [](Logger *) {}));
    assert(!manager.isSynced());
    assert(manager.effectiveStratum(2) == 2);
    return 0;
  }

  UpstreamSyncResult result =
      queryUpstreamServer("pool.ntp.org", std::chrono::milliseconds(2000));
  assert(result.success);
  assert(result.stratum >= 1 && result.stratum <= 15);
  assert(result.delay_us > 0);

  auto config = std::make_shared<NtpConfig>();
  config->upstream_servers = {"pool.ntp.org"};
  config->sync_interval = std::chrono::seconds(3600);
  config->timeout = std::chrono::milliseconds(2000);

  auto &logger = Logger::getInstance();
  logger.setLevel(LogLevel::ERROR);
  UpstreamSyncManager manager(config,
                              std::shared_ptr<Logger>(&logger, [](Logger *) {}));
  const UpstreamSyncResult once = manager.syncOnce();
  assert(once.success);
  assert(manager.isSynced());

  std::cout << "Upstream sync tests passed (offset_us=" << once.offset_us << ")."
            << std::endl;
  return 0;
}
