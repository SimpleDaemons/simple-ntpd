/**
 * @file platform.hpp
 * @brief Platform abstraction layer
 * @author SimpleDaemons
 * @copyright 2024 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace simple_ntpd {

/**
 * @brief Platform abstraction layer
 *
 * This header provides platform-specific definitions and abstractions
 * for the NTP daemon to work across different operating systems.
 */

#ifdef _WIN32
#define PLATFORM_WINDOWS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Windows-specific types
using socket_t = SOCKET;
using ssize_t = SSIZE_T;

// Windows socket error handling
#define SOCKET_ERROR_CODE WSAGetLastError()
#define CLOSE_SOCKET closesocket

// Windows-specific constants
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define INVALID_SOCKET INVALID_SOCKET

#elif defined(__APPLE__)
#define PLATFORM_MACOS
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Unix-like types
using socket_t = int;

// Unix socket error handling
#define SOCKET_ERROR_CODE errno
#define CLOSE_SOCKET close

// Unix-specific constants
#define INVALID_SOCKET_VALUE -1
#define INVALID_SOCKET -1

#elif defined(__linux__)
#define PLATFORM_LINUX
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Unix-like types
using socket_t = int;

// Unix socket error handling
#define SOCKET_ERROR_CODE errno
#define CLOSE_SOCKET close

// Unix-specific constants
#define INVALID_SOCKET_VALUE -1
#define INVALID_SOCKET -1

#else
#error "Unsupported platform"
#endif

// Common platform-independent types
using port_t = uint16_t;
using ipv4_addr_t = uint32_t;
using ipv6_addr_t = uint8_t[16];

// NTP-specific constants
constexpr port_t NTP_DEFAULT_PORT = 123;
constexpr size_t NTP_PACKET_SIZE = 48;
constexpr size_t NTP_MAX_PACKET_SIZE = 1024;
constexpr size_t NTP_VERSION = 4;
constexpr size_t NTP_STRATUM_MAX = 15;
constexpr size_t NTP_REFERENCE_ID_LENGTH = 4;
constexpr size_t NTP_ROOT_DELAY_SCALE = 2.0;      // 2^-16 seconds
constexpr size_t NTP_ROOT_DISPERSION_SCALE = 2.0; // 2^-16 seconds

// NTP packet field sizes
constexpr size_t NTP_LEAP_INDICATOR_BITS = 2;
constexpr size_t NTP_VERSION_BITS = 3;
constexpr size_t NTP_MODE_BITS = 3;
constexpr size_t NTP_STRATUM_BITS = 8;
constexpr size_t NTP_POLL_BITS = 8;
constexpr size_t NTP_PRECISION_BITS = 8;

// NTP modes
enum class NtpMode : uint8_t {
  RESERVED = 0,
  SYMMETRIC_ACTIVE = 1,
  SYMMETRIC_PASSIVE = 2,
  CLIENT = 3,
  SERVER = 4,
  BROADCAST = 5,
  NTP_CONTROL_MESSAGE = 6,
  RESERVED_PRIVATE = 7
};

// NTP leap indicator
enum class NtpLeapIndicator : uint8_t {
  NO_WARNING = 0,
  LAST_MINUTE_61 = 1,
  LAST_MINUTE_59 = 2,
  ALARM_CONDITION = 3
};

// NTP stratum levels
enum class NtpStratum : uint8_t {
  UNSPECIFIED = 0,
  PRIMARY_REFERENCE = 1,
  SECONDARY_REFERENCE = 2,
  TERTIARY_REFERENCE = 3,
  // 4-15 are secondary servers
  MAX_STRATUM = 15
};

} // namespace simple_ntpd
