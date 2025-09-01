# Configuration Guide

Learn how to configure the Simple NTP Daemon for your environment. This guide covers all configuration options, best practices, and common scenarios.

## 📋 Table of Contents

1. [Configuration File Format](#configuration-file-format)
2. [Basic Configuration](#basic-configuration)
3. [Network Configuration](#network-configuration)
4. [NTP Server Configuration](#ntp-server-configuration)
5. [Logging Configuration](#logging-configuration)
6. [Security Configuration](#security-configuration)
7. [Performance Configuration](#performance-configuration)
8. [Configuration Examples](#configuration-examples)
9. [Environment Variables](#environment-variables)
10. [Configuration Validation](#configuration-validation)

## 📁 Configuration File Format

Simple NTP Daemon uses a simple key-value configuration format:

```ini
# This is a comment
key = value

# Multiple values can be specified
upstream_servers = pool.ntp.org, time.nist.gov

# Boolean values
enable_ipv6 = true
enable_console_logging = true
```

## 🔧 Basic Configuration

### Essential Settings

```ini
# Server identification
server_name = my-ntp-server
server_description = Internal NTP Server

# Basic operation
daemonize = false
pid_file = /var/run/simple-ntpd.pid
```

### File Paths

```ini
# Configuration and data directories
config_dir = /etc/simple-ntpd
log_dir = /var/log/simple-ntpd
data_dir = /var/lib/simple-ntpd
```

## 🌐 Network Configuration

### Listening Configuration

```ini
# Network interface binding
listen_address = 0.0.0.0          # All interfaces
# listen_address = 192.168.1.10   # Specific interface
# listen_address = ::             # IPv6 all interfaces

# Port configuration
listen_port = 123                  # Standard NTP port
enable_ipv6 = true                # Enable IPv6 support

# Connection limits
max_connections = 1000            # Maximum concurrent connections
connection_timeout = 30           # Connection timeout in seconds
```

### Upstream Servers

```ini
# Primary time sources
upstream_servers = pool.ntp.org, time.nist.gov, time.google.com

# Backup servers (optional)
backup_servers = time.cloudflare.com, time.windows.com

# Server selection
server_selection = round_robin    # round_robin, priority, random
failover_enabled = true           # Enable automatic failover
```

## ⏰ NTP Server Configuration

### Stratum and Reference

```ini
# Server stratum level
stratum = 2                       # 1-15 (1 = primary reference)

# Reference clock information
reference_id = LOCL              # 4-character reference identifier
reference_clock = LOCAL          # Reference clock description

# Synchronization settings
sync_interval = 64               # Sync interval in seconds
timeout = 1000                   # Request timeout in milliseconds
```

### Time Source Configuration

```ini
# Local time source (if available)
local_clock = false              # Use local system clock
local_clock_precision = -6       # Local clock precision

# External reference clocks
reference_clock_type = none      # none, gps, atomic, etc.
reference_clock_device = /dev/gps0
```

## 📝 Logging Configuration

### Log Levels and Output

```ini
# Log level configuration
log_level = INFO                 # DEBUG, INFO, WARNING, ERROR, FATAL

# Output destinations
enable_console_logging = true    # Console output
enable_file_logging = true       # File output
enable_syslog = false            # Syslog output

# File logging configuration
log_file = /var/log/simple-ntpd/ntpd.log
log_file_max_size = 10MB         # Maximum log file size
log_file_backups = 5             # Number of backup files
log_rotation = daily             # daily, weekly, monthly
```

### Log Format and Content

```ini
# Log format options
log_timestamp = true             # Include timestamps
log_thread_id = true             # Include thread IDs
log_source_file = true           # Include source file information
log_structured = false           # Structured (JSON) logging

# Log content filtering
log_ntp_requests = true          # Log NTP requests
log_ntp_responses = true         # Log NTP responses
log_errors = true                # Log error conditions
log_performance = false          # Log performance metrics
```

## 🔒 Security Configuration

### Access Control

```ini
# Network access control
allowed_networks = 192.168.0.0/16, 10.0.0.0/8
denied_networks = 192.168.1.100, 10.0.0.50

# Client restrictions
max_requests_per_minute = 60     # Rate limiting
enable_query_restrictions = true # Restrict query types
restrict_queries = true          # Restrict query access

# Authentication
enable_authentication = false    # NTP authentication
authentication_key = ""          # Authentication key
authentication_algorithm = md5  # md5, sha1, sha256
```

### Network Security

```ini
# DDoS protection
enable_ddos_protection = true    # Enable DDoS protection
max_connections_per_ip = 10      # Max connections per IP
connection_rate_limit = 100      # Connections per second

# Firewall integration
firewall_rules = auto            # auto, manual, none
firewall_zone = internal         # Firewall zone name
```

## ⚡ Performance Configuration

### Resource Management

```ini
# Worker threads
worker_threads = 4               # Number of worker threads
thread_pool_size = 16            # Thread pool size

# Memory management
max_memory_usage = 256MB         # Maximum memory usage
connection_buffer_size = 8KB     # Connection buffer size

# Performance tuning
enable_connection_pooling = true # Connection pooling
enable_memory_pooling = true     # Memory pooling
enable_async_io = false          # Asynchronous I/O
```

### Caching and Optimization

```ini
# Response caching
enable_response_cache = true     # Cache NTP responses
cache_ttl = 300                  # Cache TTL in seconds
max_cache_size = 1000            # Maximum cache entries

# Performance monitoring
enable_performance_monitoring = true
performance_metrics_interval = 60
```

## 📚 Configuration Examples

### Development Environment

```ini
# Development configuration
log_level = DEBUG
enable_console_logging = true
enable_file_logging = false
daemonize = false
listen_address = 127.0.0.1
max_connections = 100
```

### Production Environment

```ini
# Production configuration
log_level = WARNING
enable_console_logging = false
enable_file_logging = true
daemonize = true
listen_address = 0.0.0.0
max_connections = 10000
enable_ddos_protection = true
worker_threads = 8
```

### High-Security Environment

```ini
# High-security configuration
log_level = INFO
enable_authentication = true
authentication_algorithm = sha256
allowed_networks = 10.0.0.0/8
enable_query_restrictions = true
max_requests_per_minute = 30
enable_ddos_protection = true
```

## 🌍 Environment Variables

Configuration can be overridden using environment variables:

```bash
# Override configuration values
export SIMPLE_NTPD_LOG_LEVEL=DEBUG
export SIMPLE_NTPD_LISTEN_ADDRESS=0.0.0.0
export SIMPLE_NTPD_ENABLE_IPV6=true

# Run the daemon
./simple-ntpd -c config.conf
```

### Supported Environment Variables

| Environment Variable | Configuration Key | Example |
|---------------------|-------------------|---------|
| `SIMPLE_NTPD_LOG_LEVEL` | `log_level` | `DEBUG` |
| `SIMPLE_NTPD_LISTEN_ADDRESS` | `listen_address` | `0.0.0.0` |
| `SIMPLE_NTPD_LISTEN_PORT` | `listen_port` | `123` |
| `SIMPLE_NTPD_ENABLE_IPV6` | `enable_ipv6` | `true` |
| `SIMPLE_NTPD_STRATUM` | `stratum` | `2` |

## ✅ Configuration Validation

### Validation Commands

```bash
# Validate configuration file
./simple-ntpd --validate-config config.conf

# Check configuration syntax
./simple-ntpd --check-config config.conf

# Test configuration without starting
./simple-ntpd --dry-run -c config.conf
```

### Common Validation Errors

- **Invalid IP addresses**: Check network configuration
- **Permission issues**: Verify file and directory permissions
- **Port conflicts**: Ensure port 123 is available
- **Invalid log levels**: Use DEBUG, INFO, WARNING, ERROR, or FATAL

## 🔄 Configuration Reloading

### Dynamic Configuration Updates

```bash
# Reload configuration without restart
kill -HUP $(cat /var/run/simple-ntpd.pid)

# Or use the management interface
./simple-ntpd --reload-config
```

### Reloadable Settings

- Log level and output configuration
- Network access control lists
- Upstream server lists
- Performance tuning parameters

## 📖 Next Steps

- **Examples**: See [Configuration Examples](../examples/README.md)
- **Reference**: Check [Configuration Reference](reference.md)
- **Deployment**: Review [Production Deployment](../deployment/production.md)
- **Security**: Learn about [Security Hardening](../deployment/security.md)

---

*For a complete list of configuration options, see the [Configuration Reference](reference.md).*
