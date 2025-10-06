# Simple NTP Daemon

A lightweight, high-performance NTP (Network Time Protocol) daemon written in C++.

## Features

- **High Performance**: Optimized for low-latency time synchronization
- **Cross-Platform**: Supports macOS, Linux, and Windows
- **Multi-Format Configuration**: INI, JSON, and YAML configuration support
- **Structured Logging**: JSON structured logging with configurable output
- **Enhanced Validation**: Comprehensive NTP packet validation and error reporting
- **Comprehensive Testing**: Unit tests with automated test framework
- **Secure**: Built with security best practices
- **Containerized**: Full Docker support for development and deployment

## Quick Start

### Using Docker (Recommended)

```bash
# Build for all platforms
./scripts/build-docker.sh -d all

# Run the NTP daemon
docker-compose --profile runtime up simple-ntpd

# Development environment
docker-compose --profile dev up dev
```

### Local Development

```bash
# Install dependencies
make deps

# Build the project
make build

# Run tests
make test

# Run specific tests
make test-ntp-packet    # NTP packet functionality tests
make test-ntp-config    # Configuration system tests

# Install
make install
```

### Configuration Formats

The daemon supports multiple configuration formats:

```bash
# INI format
./simple-ntpd --config config.ini

# JSON format  
./simple-ntpd --config config.json

# YAML format
./simple-ntpd --config config.yaml
```

Example configurations are available in `config/examples/`:
- `simple-ntpd.json` - JSON configuration example
- `simple-ntpd.yaml` - YAML configuration example
- `simple-ntpd.conf` - INI configuration example

## Help System

The project includes a comprehensive modular help system:

```bash
# Quick reference
make help

# Complete reference
make help-all

# Category-specific help
make help-build       # Build and development targets
make help-package     # Package creation targets
make help-deps        # Dependency management targets
make help-service     # Service management targets
make help-docker      # Docker targets
make help-config      # Configuration management targets
make help-platform    # Platform-specific targets
```

## Testing

The project includes comprehensive unit tests:

```bash
# Run all tests
make test

# Run specific test suites
cd build && ./test_ntp_packet    # NTP packet functionality
cd build && ./test_ntp_config    # Configuration system

# Run tests with verbose output
make test-verbose

# Run tests in debug mode
make dev-test
```

## Static Binary Support

Create self-contained static binaries:

```bash
# Build static binary
make static-build

# Test static binary
make static-test

# Create static binary packages
make static-package   # Platform-specific package
make static-zip       # Cross-platform ZIP package
make static-all       # All static binary formats
```

## Docker Infrastructure

### Multi-Platform Support

The Docker setup supports multiple Linux distributions and architectures:

- **Distributions**: Ubuntu, CentOS, Alpine Linux
- **Architectures**: x86_64, arm64, armv7

### Build Profiles

```bash
# Development environment
docker-compose --profile dev up dev

# Runtime container
docker-compose --profile runtime up simple-ntpd

# Build for specific distribution
docker-compose --profile build build build-ubuntu
docker-compose --profile build build build-centos
docker-compose --profile build build build-alpine
```

### Automated Building

Use the build script for automated cross-platform building:

```bash
# Build for all distributions
./scripts/build-docker.sh -d all

# Build for specific distribution
./scripts/build-docker.sh -d ubuntu

# Clean build cache
./scripts/build-docker.sh --clean
```

## Configuration

### Multi-Format Support

The daemon supports three configuration formats with automatic detection:

#### INI Format
```ini
[ntp]
listen_address = 0.0.0.0
listen_port = 123
log_level = info
log_json = false
reference_id = LOCL
```

#### JSON Format
```json
{
  "ntp": {
    "listen_address": "0.0.0.0",
    "listen_port": 123,
    "log_level": "info",
    "log_json": true,
    "reference_id": "LOCL"
  }
}
```

#### YAML Format
```yaml
ntp:
  listen_address: "0.0.0.0"
  listen_port: 123
  log_level: "info"
  log_json: true
  reference_id: "LOCL"
```

### Configuration Features

- **Automatic Format Detection**: Based on file extension (.ini, .json, .yaml)
- **Structured Logging**: JSON output with `log_json = true`
- **Validation**: Comprehensive configuration validation
- **Error Reporting**: Detailed error messages for invalid configurations

## Development

### Prerequisites

- CMake 3.15+
- C++17 compatible compiler
- OpenSSL
- JsonCPP

### Development Tools

```bash
# Install development tools (macOS)
make dev-deps

# Install development tools (Homebrew alternative)
make dev-deps-brew

# Run code quality checks
make format
make lint
make security-scan
```

### Testing

```bash
# Run all tests
make test

# Run specific test suites
cd build && ./test_ntp_packet    # NTP packet functionality
cd build && ./test_ntp_config    # Configuration system

# Run tests with verbose output
make test-verbose

# Run tests in debug mode
make dev-test
```

## Building

### Local Build

```bash
# Standard build
make build

# Debug build
make debug

# Release build
make release

# Development build
make dev-build
```

### Package Creation

```bash
# Create platform-specific packages
make package

# Create source package
make package-source

# Create all package formats
make package-all
```

### Static Binary Support

Create self-contained static binaries:

```bash
# Build static binary
make static-build

# Test static binary
make static-test

# Create static binary packages
make static-package   # Platform-specific package
make static-zip       # Cross-platform ZIP package
make static-all       # All static binary formats
```

## Service Management

### Installation

```bash
# Install system service
make service-install

# Check service status
make service-status

# Start service
make service-start

# Stop service
make service-stop
```

### Platform Support

- **macOS**: launchd service
- **Linux**: systemd service  
- **Windows**: Windows Service

## Configuration

### Installation

```bash
# Install configuration files
make config-install

# Backup configuration
make config-backup

# Install log rotation
make log-rotate
```

## Security

The project includes comprehensive security scanning:

```bash
# Run security scan
make security-scan

# Run static analysis
make analyze

# Check code style
make check-style
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests and quality checks
5. Submit a pull request

### Code Quality

```bash
# Format code
make format

# Run linting
make lint

# Run security scan
make security-scan
```

## License

[Add your license information here]

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for a detailed history of changes.
