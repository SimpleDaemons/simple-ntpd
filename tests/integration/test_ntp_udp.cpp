/**
 * @file test_ntp_udp.cpp
 * @brief UDP integration test against a live local NTP server instance
 */

#include "simple-ntpd/core/packet.hpp"
#include "simple-ntpd/core/server.hpp"
#include "simple-ntpd/utils/logger.hpp"
#include <arpa/inet.h>
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace simple_ntpd;

namespace {
constexpr uint16_t kTestPort = 9123;
}

int main() {
  std::cout << "Running NTP UDP Integration Tests..." << std::endl;

  auto config = std::make_shared<NtpConfig>();
  config->listen_address = "127.0.0.1";
  config->listen_port = kTestPort;
  config->upstream_servers.clear();
  config->enable_leap_second_handling = false;
  config->enable_console_logging = false;
  config->worker_threads = 1;
  config->log_level = LogLevel::ERROR;
#ifdef __APPLE__
  config->log_file = "/dev/null";
#else
  config->log_file = "/dev/null";
#endif

  auto &logger = Logger::getInstance();
  logger.setLevel(LogLevel::ERROR);
  logger.setDestination(LogDestination::CONSOLE);

  auto server = std::make_shared<NtpServer>(
      config, std::shared_ptr<Logger>(&logger, [](Logger *) {}));

  assert(server->start());

  std::thread server_thread([&server]() {
    while (server->isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(sock != INVALID_SOCKET);

  struct sockaddr_in dest {};
  dest.sin_family = AF_INET;
  dest.sin_port = htons(kTestPort);
  assert(inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr) == 1);

  NtpPacket request = NtpPacket::createClientRequest();
  const auto request_data = request.serializeToData();
  ssize_t sent = sendto(sock, request_data.data(), request_data.size(), 0,
                        reinterpret_cast<struct sockaddr *>(&dest), sizeof(dest));
  assert(sent == static_cast<ssize_t>(request_data.size()));

  std::array<uint8_t, NTP_PACKET_SIZE> buffer{};
  struct sockaddr_in from {};
  socklen_t from_len = sizeof(from);
  struct timeval tv {2, 0};
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  const ssize_t received =
      recvfrom(sock, buffer.data(), buffer.size(), 0,
               reinterpret_cast<struct sockaddr *>(&from), &from_len);
  close(sock);
  assert(received == static_cast<ssize_t>(NTP_PACKET_SIZE));

  NtpPacket response;
  std::vector<uint8_t> response_data(buffer.begin(), buffer.end());
  assert(response.parseFromData(response_data));
  assert(response.isValid());
  assert(response.mode == static_cast<uint8_t>(NtpMode::SERVER));
  assert(response.stratum >= 1);

  server->stop();
  server_thread.join();

  std::cout << "UDP integration tests passed." << std::endl;
  return 0;
}
