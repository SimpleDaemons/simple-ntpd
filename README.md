# Simple NTP Daemon (simple-ntpd)

A lightweight, cross-platform Network Time Protocol (NTP) daemon written in C++.

## Overview

Simple NTP Daemon is a high-performance NTP server implementation that provides accurate time synchronization services. It's designed to be lightweight, secure, and easy to configure while maintaining compatibility with the NTP protocol (RFC 5905).

## Features

- **Cross-platform support**: macOS, Linux (Debian/RedHat/Generic), and Windows
- **NTP v4 protocol compliance**: Full implementation of RFC 5905
- **High performance**: Multi-threaded architecture with efficient packet handling
- **Configurable stratum levels**: Support for primary and secondary time servers
- **IPv6 support**: Native IPv6 support alongside IPv4
- **Security features**: Configurable access control and authentication
- **Comprehensive logging**: Configurable log levels with file and console output
- **Statistics collection**: Detailed server and connection statistics
- **Drift compensation**: Automatic clock drift detection and compensation
- **Leap second handling**: Proper handling of leap second announcements

## Requirements

- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.16+**
- **Platform-specific libraries**:
  - **macOS**: Security, CoreFoundation, SystemConfiguration frameworks
  - **Linux**: librt
  - **Windows**: ws2_32, iphlpapi

## Building

### Prerequisites

1. Install CMake 3.16 or later
2. Install a C++17 compatible compiler
3. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/simple-ntpd.git
   cd simple-ntpd
   ```

### Build Commands

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

# Install (optional)
sudo make install
```

### Build Options

- `BUILD_TESTS`: Enable test suite (default: ON)
- `BUILD_EXAMPLES`: Enable example programs (default: OFF)
- `ENABLE_LOGGING`: Enable logging system (default: ON)
- `ENABLE_IPV6`: Enable IPv6 support (default: ON)

### Platform-Specific Builds

#### macOS
```bash
# Build for macOS with universal binary support
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..
make
```

#### Linux
```bash
# Build for Linux
cmake ..
make
```

#### Windows
```bash
# Build for Windows (using Visual Studio)
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## Configuration

### Configuration File

The daemon can be configured using a configuration file. An example configuration is provided at `config/simple-ntpd.conf.example`.

Key configuration options:

```ini
# Network settings
listen_address = 0.0.0.0
listen_port = 123

# NTP server settings
stratum = 2
reference_id = "GPS"
upstream_servers = ["time.nist.gov", "pool.ntp.org"]

# Logging
log_level = INFO
log_file = /var/log/simple-ntpd.log

# Security
enable_authentication = false
restrict_queries = false
```

### Command Line Options

```bash
# Basic usage
simple-ntpd start

# With configuration file
simple-ntpd start --config /etc/simple-ntpd/config.conf

# Custom settings
simple-ntpd start --listen 0.0.0.0 --port 123 --stratum 2

# Verbose logging
simple-ntpd start --verbose

# Run as daemon
simple-ntpd --daemon start

# Test configuration
simple-ntpd --test-config --config /etc/simple-ntpd/config.conf
```

## Usage

### Starting the Server

```bash
# Start with default configuration
sudo simple-ntpd start

# Start with custom configuration
sudo simple-ntpd start --config /etc/simple-ntpd/config.conf

# Start in foreground for debugging
sudo simple-ntpd --foreground start
```

### Testing the Server

```bash
# Test with ntpdate (if available)
sudo ntpdate -q localhost

# Test with ntpq
ntpq -p localhost

# Test with chrony
chronyc sources -v
```

### Server Management

```bash
# Check server status
simple-ntpd status

# View statistics
simple-ntpd stats

# List active connections
simple-ntpd connections

# Reload configuration
simple-ntpd reload
```

## Architecture

The daemon is built with a modular architecture:

- **Platform Layer**: Cross-platform abstractions for networking and system calls
- **Packet Handler**: NTP protocol packet parsing and creation
- **Connection Manager**: Individual client connection handling
- **Server Core**: Main server logic and connection acceptance
- **Configuration**: Flexible configuration management
- **Logging**: Comprehensive logging system

## Protocol Support

- **NTP v4**: Full RFC 5905 compliance
- **Client Mode**: Responds to client requests
- **Server Mode**: Acts as a time server
- **Symmetric Mode**: Peer-to-peer time synchronization
- **Broadcast Mode**: Broadcast time information

## Security Considerations

- **Access Control**: Configurable client allow/deny lists
- **Authentication**: Optional MD5 authentication support
- **Rate Limiting**: Configurable request rate limiting
- **Input Validation**: Comprehensive packet validation
- **Privilege Separation**: Runs with minimal required privileges

## Performance

- **Multi-threading**: Efficient handling of multiple concurrent clients
- **Memory Management**: Optimized memory allocation and deallocation
- **Network I/O**: Non-blocking socket operations
- **Statistics Collection**: Minimal overhead statistics gathering

## Monitoring and Logging

### Log Levels

- **DEBUG**: Detailed debugging information
- **INFO**: General operational information
- **WARNING**: Warning messages
- **ERROR**: Error conditions
- **FATAL**: Fatal errors

### Statistics

The daemon provides comprehensive statistics:

- Connection counts and rates
- Packet processing statistics
- Error rates and types
- Performance metrics
- Client activity tracking

## Troubleshooting

### Common Issues

1. **Permission Denied**: Ensure the daemon has permission to bind to port 123
2. **Port Already in Use**: Check if another NTP service is running
3. **Configuration Errors**: Validate configuration with `--test-config`
4. **Network Issues**: Verify firewall settings and network connectivity

### Debug Mode

```bash
# Enable verbose logging
simple-ntpd --verbose start

# Run in foreground for debugging
simple-ntpd --foreground start
```

## Contributing

Contributions are welcome! Please see the [Contributing Guide](CONTRIBUTING.md) for details.

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- RFC 5905 authors for the NTP protocol specification
- The open source community for inspiration and tools
- Contributors and testers

## Support

For support and questions:

- **Issues**: [GitHub Issues](https://github.com/yourusername/simple-ntpd/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/simple-ntpd/discussions)
- **Email**: contact@blburns.com

## Roadmap

- [ ] Authentication improvements
- [ ] SNMP monitoring support
- [ ] Web-based management interface
- [ ] Enhanced security features
- [ ] Performance optimizations
- [ ] Additional platform support

---

**Note**: This is a development version. For production use, please ensure thorough testing and validation.
