# Command Line Interface

This document describes the command line interface for Simple NTP Daemon, including all available options, arguments, and usage examples.

## Overview

Simple NTP Daemon provides a comprehensive command line interface for:
- Starting and stopping the service
- Configuration validation and testing
- Debugging and troubleshooting
- Service management and control

## Basic Usage

### Command Syntax

```bash
simple-ntpd [OPTIONS] [COMMAND]
```

### Common Commands

```bash
# Start the service
simple-ntpd start

# Start with specific configuration
simple-ntpd start --config /path/to/config.conf

# Run in foreground mode
simple-ntpd --foreground

# Test configuration
simple-ntpd --test-config --config /path/to/config.conf

# Show version information
simple-ntpd --version

# Show help
simple-ntpd --help
```

## Command Options

### Global Options

| Option | Long Form | Description | Default |
|--------|-----------|-------------|---------|
| `-c` | `--config` | Configuration file path | `/etc/simple-ntpd/simple-ntpd.conf` |
| `-v` | `--verbose` | Enable verbose logging | `false` |
| `-q` | `--quiet` | Suppress non-error output | `false` |
| `-f` | `--foreground` | Run in foreground mode | `false` |
| `-d` | `--daemon` | Run as daemon (background) | `true` |
| `-p` | `--pid-file` | PID file path | `/var/run/simple-ntpd/simple-ntpd.pid` |
| `-u` | `--user` | User to run as | `ntp` |
| `-g` | `--group` | Group to run as | `ntp` |
| `-l` | `--log-level` | Log level (DEBUG, INFO, WARN, ERROR) | `INFO` |
| `-o` | `--log-output` | Log output (file, console, syslog) | `file` |
| `-s` | `--socket-path` | Unix socket path | `/var/run/simple-ntpd/simple-ntpd.sock` |
| `-V` | `--version` | Show version information | `false` |
| `-h` | `--help` | Show help information | `false` |

### Service Control Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-S` | `--start` | Start the service |
| `-K` | `--stop` | Stop the service |
| `-R` | `--restart` | Restart the service |
| `-H` | `--reload` | Reload configuration |
| `-T` | `--status` | Show service status |
| `-I` | `--info` | Show service information |

### Configuration Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-t` | `--test-config` | Test configuration file |
| `-V` | `--validate-config` | Validate configuration syntax |
| `-D` | `--dump-config` | Dump current configuration |
| `-e` | `--edit-config` | Edit configuration file |
| `-b` | `--backup-config` | Backup current configuration |

### Debug Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-D` | `--debug` | Enable debug mode |
| `-L` | `--log-file` | Specify log file path |
| `-P` | `--profile` | Enable profiling |
| `-M` | `--memory-check` | Enable memory checking |
| `-T` | `--trace` | Enable function tracing |

## Command Examples

### Starting the Service

#### Basic Start
```bash
# Start with default configuration
sudo simple-ntpd start

# Start with specific configuration file
sudo simple-ntpd start --config /etc/simple-ntpd/custom.conf

# Start with verbose logging
sudo simple-ntpd start --verbose --log-level DEBUG
```

#### Foreground Mode
```bash
# Run in foreground for debugging
sudo simple-ntpd --foreground --verbose

# Run with specific configuration in foreground
sudo simple-ntpd --foreground --config /etc/simple-ntpd/debug.conf

# Run with debug logging in foreground
sudo simple-ntpd --foreground --log-level DEBUG --log-output console
```

#### Daemon Mode
```bash
# Run as daemon (default)
sudo simple-ntpd start --daemon

# Run as daemon with specific user
sudo simple-ntpd start --daemon --user ntp --group ntp

# Run as daemon with PID file
sudo simple-ntpd start --daemon --pid-file /var/run/simple-ntpd.pid
```

### Configuration Management

#### Test Configuration
```bash
# Test default configuration
sudo simple-ntpd --test-config

# Test specific configuration file
sudo simple-ntpd --test-config --config /etc/simple-ntpd/test.conf

# Test configuration with verbose output
sudo simple-ntpd --test-config --config /etc/simple-ntpd/test.conf --verbose
```

#### Validate Configuration
```bash
# Validate configuration syntax
sudo simple-ntpd --validate-config --config /etc/simple-ntpd/config.conf

# Validate and show errors
sudo simple-ntpd --validate-config --config /etc/simple-ntpd/config.conf --verbose
```

#### Dump Configuration
```bash
# Dump current configuration
sudo simple-ntpd --dump-config

# Dump configuration to file
sudo simple-ntpd --dump-config > current-config.conf

# Dump configuration with comments
sudo simple-ntpd --dump-config --include-comments > documented-config.conf
```

### Service Control

#### Start Service
```bash
# Start service
sudo simple-ntpd start

# Start with specific options
sudo simple-ntpd start --config /etc/simple-ntpd/production.conf --user ntp --group ntp

# Start and enable auto-start
sudo simple-ntpd start --enable
```

#### Stop Service
```bash
# Stop service gracefully
sudo simple-ntpd stop

# Force stop service
sudo simple-ntpd stop --force

# Stop and disable auto-start
sudo simple-ntpd stop --disable
```

#### Restart Service
```bash
# Restart service
sudo simple-ntpd restart

# Restart with new configuration
sudo simple-ntpd restart --config /etc/simple-ntpd/new.conf

# Restart and reload configuration
sudo simple-ntpd restart --reload
```

#### Service Status
```bash
# Check service status
sudo simple-ntpd status

# Show detailed status
sudo simple-ntpd status --detailed

# Show status in JSON format
sudo simple-ntpd status --format json
```

### Debugging and Troubleshooting

#### Debug Mode
```bash
# Enable debug mode
sudo simple-ntpd --debug --foreground

# Debug with specific log level
sudo simple-ntpd --debug --log-level DEBUG --foreground

# Debug with memory checking
sudo simple-ntpd --debug --memory-check --foreground
```

#### Logging Options
```bash
# Set log level
sudo simple-ntpd --log-level DEBUG

# Set log output
sudo simple-ntpd --log-output console

# Specify log file
sudo simple-ntpd --log-file /tmp/debug.log

# Enable syslog
sudo simple-ntpd --log-output syslog
```

#### Profiling
```bash
# Enable profiling
sudo simple-ntpd --profile --foreground

# Profile with specific options
sudo simple-ntpd --profile --profile-file /tmp/profile.log --foreground

# Profile memory usage
sudo simple-ntpd --profile --memory-profile --foreground
```

## Environment Variables

You can override command line options using environment variables:

```bash
# Set configuration file
export SIMPLE_NTPD_CONFIG="/etc/simple-ntpd/custom.conf"

# Set log level
export SIMPLE_NTPD_LOG_LEVEL="DEBUG"

# Set user and group
export SIMPLE_NTPD_USER="ntp"
export SIMPLE_NTPD_GROUP="ntp"

# Set log output
export SIMPLE_NTPD_LOG_OUTPUT="console"

# Set socket path
export SIMPLE_NTPD_SOCKET_PATH="/tmp/ntpd.sock"
```

## Configuration File Override

Command line options take precedence over configuration file settings:

```bash
# Override configuration file settings
sudo simple-ntpd start \
  --config /etc/simple-ntpd/config.conf \
  --log-level DEBUG \
  --log-output console \
  --user ntp \
  --group ntp
```

## Exit Codes

Simple NTP Daemon uses the following exit codes:

| Code | Description |
|------|-------------|
| `0` | Success |
| `1` | General error |
| `2` | Configuration error |
| `3` | Permission denied |
| `4` | Service already running |
| `5` | Service not running |
| `6` | Invalid command line arguments |
| `7` | File not found |
| `8` | Network error |
| `9` | System resource error |

## Usage Examples

### Development Environment
```bash
# Start in development mode
sudo simple-ntpd start \
  --config /etc/simple-ntpd/dev.conf \
  --log-level DEBUG \
  --log-output console \
  --foreground
```

### Production Environment
```bash
# Start in production mode
sudo simple-ntpd start \
  --config /etc/simple-ntpd/production.conf \
  --log-level INFO \
  --log-output file \
  --daemon \
  --user ntp \
  --group ntp
```

### Testing Configuration
```bash
# Test configuration before applying
sudo simple-ntpd --test-config --config /etc/simple-ntpd/test.conf

# Validate configuration syntax
sudo simple-ntpd --validate-config --config /etc/simple-ntpd/test.conf

# Dump configuration for review
sudo simple-ntpd --dump-config --config /etc/simple-ntpd/test.conf
```

### Troubleshooting
```bash
# Run in foreground with debug logging
sudo simple-ntpd --foreground --log-level DEBUG --log-output console

# Check service status
sudo simple-ntpd status

# Reload configuration
sudo simple-ntpd reload
```

## Integration with System Services

### systemd Integration
```bash
# Start via systemd
sudo systemctl start simple-ntpd

# Check status via systemd
sudo systemctl status simple-ntpd

# Reload configuration via systemd
sudo systemctl reload simple-ntpd
```

### init.d Integration
```bash
# Start via init.d
sudo /etc/init.d/simple-ntpd start

# Check status via init.d
sudo /etc/init.d/simple-ntpd status

# Reload configuration via init.d
sudo /etc/init.d/simple-ntpd reload
```

## Best Practices

### Command Line Usage
1. **Always test configuration** before starting the service
2. **Use appropriate log levels** for your environment
3. **Specify configuration files** explicitly when possible
4. **Use environment variables** for sensitive information
5. **Run in foreground mode** for debugging

### Service Management
1. **Use system service managers** (systemd, init.d) in production
2. **Test configuration changes** before applying
3. **Monitor service status** regularly
4. **Use appropriate user/group** for security
5. **Backup configuration** before making changes

### Debugging
1. **Start with verbose logging** to identify issues
2. **Use foreground mode** for interactive debugging
3. **Check exit codes** for error identification
4. **Review logs** for detailed error information
5. **Test configuration** before troubleshooting

## Troubleshooting

### Common Issues

#### Permission Denied
```bash
# Check user permissions
sudo simple-ntpd start --user ntp --group ntp

# Check file permissions
ls -la /etc/simple-ntpd/
ls -la /var/log/simple-ntpd/
```

#### Configuration Errors
```bash
# Test configuration
sudo simple-ntpd --test-config --config /etc/simple-ntpd/config.conf

# Validate syntax
sudo simple-ntpd --validate-config --config /etc/simple-ntpd/config.conf
```

#### Service Already Running
```bash
# Check if service is running
sudo simple-ntpd status

# Stop existing service
sudo simple-ntpd stop

# Start new service
sudo simple-ntpd start
```

### Getting Help

```bash
# Show help
simple-ntpd --help

# Show version
simple-ntpd --version

# Show configuration help
simple-ntpd --config-help
```

## Conclusion

The Simple NTP Daemon command line interface provides comprehensive control over the service, from basic operation to advanced debugging. By understanding the available options and commands, you can effectively manage and troubleshoot your NTP infrastructure.

Remember to:
- Always test configuration changes before applying
- Use appropriate log levels for your environment
- Leverage system service managers in production
- Use debugging options for troubleshooting
- Follow security best practices

For additional help, refer to the main documentation and community resources.
