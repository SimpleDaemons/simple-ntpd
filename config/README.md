# Simple NTP Daemon Configuration Guide

This directory contains configuration files and examples for the Simple NTP Daemon.

## Directory Structure

```
config/
├── README.md              # This file
└── examples/              # Configuration examples
    ├── simple/            # Simple configurations
    │   └── simple-ntpd.conf.example
    ├── production/        # Production-ready configurations
    └── advanced/          # Advanced configurations with custom options
```

## Configuration File

The main configuration file is `simple-ntpd.conf` which should be placed in `/etc/simple-ntpd/` after installation.

### Configuration Sections

#### Network Configuration
```ini
[network]
# Listen address (0.0.0.0 for all interfaces)
listen_address = 0.0.0.0
# Listen port (standard NTP port is 123)
listen_port = 123
# Enable IPv6 support
enable_ipv6 = true
# Maximum number of concurrent connections
max_connections = 1000
```

#### NTP Server Configuration
```ini
[ntp_server]
# Stratum level (1 = primary server, 2+ = secondary)
stratum = 2
# Reference clock identifier
reference_clock = LOCAL
# Upstream NTP servers for synchronization
upstream_servers = pool.ntp.org, time.nist.gov
# Minimum polling interval (seconds)
min_poll = 4
# Maximum polling interval (seconds)
max_poll = 17
```

#### Logging Configuration
```ini
[logging]
# Log level (DEBUG, INFO, WARNING, ERROR, FATAL)
level = INFO
# Log destination (console, file, syslog)
destination = file
# Log file path (if destination is file)
log_file = /var/log/simple-ntpd/simple-ntpd.log
# Enable syslog integration
enable_syslog = true
# Syslog facility
syslog_facility = daemon
```

#### Security Configuration
```ini
[security]
# Enable authentication
enable_auth = false
# Authentication key file
auth_key_file = /etc/simple-ntpd/keys
# Access control list
allowed_clients = 0.0.0.0/0
# Deny list
denied_clients = 
# Rate limiting (requests per second per client)
rate_limit = 100
```

#### Performance Configuration
```ini
[performance]
# Number of worker threads
worker_threads = 4
# Maximum packet size (bytes)
max_packet_size = 1024
# Connection timeout (seconds)
connection_timeout = 30
# Enable connection pooling
enable_connection_pool = true
# Pool size
connection_pool_size = 100
```

#### Drift File Configuration
```ini
[drift_file]
# Enable drift file
enable = true
# Drift file path
path = /var/lib/simple-ntpd/drift
# Update interval (seconds)
update_interval = 3600
```

#### Leap Second Configuration
```ini
[leap_seconds]
# Enable leap second handling
enable = true
# Leap second file path
file_path = /var/lib/simple-ntpd/leap-seconds.list
# Auto-update leap second file
auto_update = true
# Update URL
update_url = https://hpiers.obspm.fr/iers/bul/bulc/ntp/leap-seconds.list
```

## Example Configurations

### Simple Configuration
The `examples/simple/simple-ntpd.conf.example` provides a basic configuration suitable for most use cases.

### Production Configuration
Production configurations include:
- Optimized performance settings
- Enhanced security options
- Comprehensive logging
- Monitoring and alerting setup

### Advanced Configuration
Advanced configurations feature:
- Custom NTP algorithms
- Multiple reference clocks
- Complex access control rules
- Performance tuning parameters

## Environment Variables

Configuration values can be overridden using environment variables:

```bash
# Network settings
export SIMPLE_NTPD_LISTEN_ADDRESS="0.0.0.0"
export SIMPLE_NTPD_LISTEN_PORT="123"
export SIMPLE_NTPD_ENABLE_IPV6="true"

# NTP server settings
export SIMPLE_NTPD_STRATUM="2"
export SIMPLE_NTPD_REFERENCE_CLOCK="LOCAL"
export SIMPLE_NTPD_UPSTREAM_SERVERS="pool.ntp.org,time.nist.gov"

# Logging settings
export SIMPLE_NTPD_LOG_LEVEL="INFO"
export SIMPLE_NTPD_LOG_DESTINATION="file"
export SIMPLE_NTPD_LOG_FILE="/var/log/simple-ntpd/simple-ntpd.log"

# Security settings
export SIMPLE_NTPD_ENABLE_AUTH="false"
export SIMPLE_NTPD_RATE_LIMIT="100"

# Performance settings
export SIMPLE_NTPD_WORKER_THREADS="4"
export SIMPLE_NTPD_MAX_CONNECTIONS="1000"
```

## Configuration Validation

The daemon validates configuration files on startup. Common validation checks include:

- File format and syntax
- Required fields presence
- Value ranges and types
- File permissions and accessibility
- Network address validity
- Port number ranges

## Configuration Reloading

The daemon supports configuration reloading without restart:

```bash
# Systemd
sudo systemctl reload simple-ntpd

# Init.d
sudo /etc/init.d/simple-ntpd reload

# Signal-based (SIGHUP)
sudo kill -HUP $(pidof simple-ntpd)
```

## Configuration Testing

Test configuration files before deployment:

```bash
# Test configuration syntax
simple-ntpd --test-config --config /path/to/config.conf

# Validate configuration
simple-ntpd --validate-config --config /path/to/config.conf

# Dry run with configuration
simple-ntpd --dry-run --config /path/to/config.conf
```

## Best Practices

### Security
- Use specific IP ranges for access control
- Enable authentication for sensitive deployments
- Implement rate limiting to prevent abuse
- Use dedicated service user/group

### Performance
- Adjust worker threads based on CPU cores
- Monitor connection pool usage
- Tune polling intervals for your network
- Use appropriate packet sizes

### Monitoring
- Enable comprehensive logging
- Set up log rotation
- Monitor drift file updates
- Track upstream server health

### Maintenance
- Regular configuration backups
- Version control for configurations
- Documentation of custom settings
- Regular security reviews

## Troubleshooting

### Common Issues

1. **Configuration File Not Found**
   ```bash
   # Check file location and permissions
   ls -la /etc/simple-ntpd/simple-ntpd.conf
   # Verify daemon can read the file
   sudo -u ntp cat /etc/simple-ntpd/simple-ntpd.conf
   ```

2. **Invalid Configuration Values**
   ```bash
   # Check configuration syntax
   simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf
   # Review error messages in logs
   tail -f /var/log/simple-ntpd/simple-ntpd.log
   ```

3. **Permission Denied**
   ```bash
   # Check file ownership
   ls -la /etc/simple-ntpd/
   # Fix permissions if needed
   sudo chown ntp:ntp /etc/simple-ntpd/simple-ntpd.conf
   sudo chmod 644 /etc/simple-ntpd/simple-ntpd.conf
   ```

4. **Configuration Reload Failed**
   ```bash
   # Check daemon status
   sudo systemctl status simple-ntpd
   # Review system logs
   sudo journalctl -u simple-ntpd -f
   ```

### Debug Mode

Enable debug logging for troubleshooting:

```ini
[logging]
level = DEBUG
destination = console
```

Run in foreground mode to see real-time output:

```bash
sudo simple-ntpd --verbose --foreground --config /etc/simple-ntpd/simple-ntpd.conf
```

## Support

For configuration issues:

- Check the troubleshooting section above
- Review system logs and daemon output
- Validate configuration syntax
- Test with minimal configuration
- Consult the main project documentation

For additional help, refer to the main project README or create an issue in the project repository.
