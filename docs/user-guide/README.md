# User Manual

This comprehensive user manual covers all aspects of using Simple NTP Daemon, from basic operation to advanced features.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Basic Operation](#basic-operation)
3. [Configuration Management](#configuration-management)
4. [Service Management](#service-management)
5. [Monitoring and Logging](#monitoring-and-logging)
6. [Client Configuration](#client-configuration)
7. [Advanced Features](#advanced-features)
8. [Troubleshooting](#troubleshooting)
9. [Best Practices](#best-practices)

## Getting Started

### What is Simple NTP Daemon?

Simple NTP Daemon is a lightweight, high-performance Network Time Protocol (NTP) server implementation designed for modern systems. It provides accurate time synchronization services with minimal resource usage and maximum reliability.

### Key Features

- **High Performance**: Optimized for high-throughput environments
- **Low Resource Usage**: Minimal CPU and memory footprint
- **Security Focused**: Built-in security features and hardening
- **Multi-Platform**: Runs on Linux, macOS, Windows, and containers
- **Easy Configuration**: Simple, human-readable configuration format
- **Monitoring Ready**: Built-in metrics and health checks

### System Requirements

- **Operating System**: Linux 3.2+, macOS 10.14+, Windows 10+
- **Architecture**: x86_64, ARM64, ARM32
- **Memory**: 128MB minimum, 512MB recommended
- **Storage**: 100MB minimum
- **Network**: UDP port 123 available

## Basic Operation

### Starting the Service

#### Linux (systemd)
```bash
# Start service
sudo systemctl start simple-ntpd

# Check status
sudo systemctl status simple-ntpd

# Enable auto-start
sudo systemctl enable simple-ntpd
```

#### Linux (init.d)
```bash
# Start service
sudo /etc/init.d/simple-ntpd start

# Check status
sudo /etc/init.d/simple-ntpd status

# Enable auto-start
sudo update-rc.d simple-ntpd defaults
```

#### macOS
```bash
# Start service
sudo launchctl start com.simpledaemons.simple-ntpd

# Check status
sudo launchctl list | grep simple-ntpd
```

#### Windows
```cmd
# Start service
net start simple-ntpd

# Check status
sc query simple-ntpd
```

### Basic Commands

```bash
# Check service status
sudo systemctl status simple-ntpd

# View logs
sudo journalctl -u simple-ntpd -f

# Test NTP service
ntpdate -q localhost

# Check listening ports
sudo netstat -ulnp | grep :123
```

## Configuration Management

### Configuration File Structure

The configuration file uses an INI-style format with sections and key-value pairs:

```ini
[section_name]
key = value
list_key = ["value1", "value2", "value3"]
```

### Main Configuration Sections

#### Network Configuration
```ini
[network]
listen_address = 0.0.0.0        # Bind address
listen_port = 123               # Bind port
enable_ipv6 = true             # Enable IPv6 support
max_clients = 1000             # Maximum concurrent clients
bind_interface = ""            # Specific network interface
```

#### NTP Server Configuration
```ini
[ntp_server]
stratum = 2                    # Stratum level
reference_clock = "LOCAL"      # Reference clock type
upstream_servers = [           # Upstream NTP servers
    "pool.ntp.org",
    "time.nist.gov"
]
sync_interval = 60             # Synchronization interval (seconds)
```

#### Logging Configuration
```ini
[logging]
level = "INFO"                 # Log level (DEBUG, INFO, WARN, ERROR)
output = "file"                # Output destination (file, console, syslog)
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
max_file_size = "10MB"         # Maximum log file size
max_files = 5                  # Number of log files to keep
```

#### Security Configuration
```ini
[security]
enable_authentication = false  # Enable NTP authentication
allow_query_from = ["0.0.0.0/0"]  # Allowed query sources
deny_query_from = []           # Denied query sources
enable_rate_limiting = true    # Enable rate limiting
max_queries_per_minute = 1000 # Rate limit threshold
```

#### Performance Configuration
```ini
[performance]
worker_threads = 4             # Number of worker threads
max_packet_size = 1024         # Maximum packet size
connection_timeout = 30        # Connection timeout (seconds)
idle_timeout = 300             # Idle connection timeout
```

### Environment Variables

You can override configuration values using environment variables:

```bash
export SIMPLE_NTPD_LISTEN_ADDRESS="0.0.0.0"
export SIMPLE_NTPD_LISTEN_PORT="123"
export SIMPLE_NTPD_STRATUM="2"
export SIMPLE_NTPD_LOG_LEVEL="INFO"
export SIMPLE_NTPD_WORKER_THREADS="8"
```

### Configuration Validation

Always validate your configuration before applying:

```bash
# Test configuration file
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf

# Check for syntax errors
sudo simple-ntpd --validate-config --config /etc/simple-ntpd/simple-ntpd.conf
```

## Service Management

### Starting and Stopping

```bash
# Start service
sudo systemctl start simple-ntpd

# Stop service
sudo systemctl stop simple-ntpd

# Restart service
sudo systemctl restart simple-ntpd

# Reload configuration
sudo systemctl reload simple-ntpd
```

### Service Status

```bash
# Check service status
sudo systemctl status simple-ntpd

# Check if service is active
sudo systemctl is-active simple-ntpd

# Check if service is enabled
sudo systemctl is-enabled simple-ntpd
```

### Service Logs

```bash
# View recent logs
sudo journalctl -u simple-ntpd -n 50

# Follow logs in real-time
sudo journalctl -u simple-ntpd -f

# View logs since specific time
sudo journalctl -u simple-ntpd --since="1 hour ago"

# View logs for specific time period
sudo journalctl -u simple-ntpd --since="2024-01-01" --until="2024-01-02"
```

### Service Control

```bash
# Enable service to start on boot
sudo systemctl enable simple-ntpd

# Disable service from starting on boot
sudo systemctl disable simple-ntpd

# Mask service (prevent starting)
sudo systemctl mask simple-ntpd

# Unmask service
sudo systemctl unmask simple-ntpd
```

## Monitoring and Logging

### Log Levels

- **DEBUG**: Detailed debugging information
- **INFO**: General information about service operation
- **WARN**: Warning messages for potential issues
- **ERROR**: Error messages for actual problems

### Log Output Formats

#### File Output
```ini
[logging]
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
log_format = "text"  # or "json"
```

#### Console Output
```ini
[logging]
output = "console"
log_format = "text"
```

#### Syslog Output
```ini
[logging]
output = "syslog"
syslog_facility = "daemon"
```

### Log Rotation

Configure log rotation to prevent log files from growing too large:

```bash
# Copy logrotate configuration
sudo cp deployment/templates/logrotate.conf /etc/logrotate.d/simple-ntpd

# Test logrotate configuration
sudo logrotate -d /etc/logrotate.d/simple-ntpd

# Force log rotation
sudo logrotate -f /etc/logrotate.d/simple-ntpd
```

### Health Checks

Simple NTP Daemon provides built-in health check endpoints:

```bash
# Health check endpoint
curl http://localhost:8080/health

# Metrics endpoint
curl http://localhost:8080/metrics

# Status endpoint
curl http://localhost:8080/status
```

### Performance Monitoring

Monitor key performance metrics:

```bash
# Check active connections
sudo netstat -an | grep :123 | wc -l

# Monitor system resources
htop
iotop

# Check NTP statistics
ntpq -p
ntpstat
```

## Client Configuration

### Linux Clients

#### Using ntpdate
```bash
# Manual time sync
sudo ntpdate your-ntp-server-ip

# Test query
ntpdate -q your-ntp-server-ip
```

#### Using ntpd
```bash
# Edit /etc/ntp.conf
server your-ntp-server-ip iburst

# Restart ntpd
sudo systemctl restart ntp
```

#### Using systemd-timesyncd
```bash
# Enable NTP synchronization
sudo timedatectl set-ntp true

# Check status
timedatectl status
```

### Windows Clients

#### Command Line
```cmd
# Set NTP server
w32tm /config /manualpeerlist:"your-ntp-server-ip" /syncfromflags:manual

# Update configuration
w32tm /config /update

# Resynchronize
w32tm /resync

# Check status
w32tm /query /status
```

#### GUI Configuration
1. Open Settings → Time & Language → Date & Time
2. Click "Additional settings" → "Set time automatically"
3. Click "Add servers for Internet time"
4. Add your NTP server IP address

### macOS Clients

```bash
# Set NTP server
sudo systemsetup -setnetworktimeserver your-ntp-server-ip

# Check NTP server
sudo systemsetup -getnetworktimeserver

# Enable NTP synchronization
sudo systemsetup -setusingnetworktime on
```

### Network Devices

Most network devices support NTP configuration:

#### Cisco Devices
```
configure terminal
ntp server your-ntp-server-ip
ntp update-calendar
end
```

#### Juniper Devices
```
set system ntp server your-ntp-server-ip
commit
```

#### MikroTik Devices
```
/system ntp client set enabled=yes primary-ntp=your-ntp-server-ip
```

## Advanced Features

### Authentication

Enable NTP authentication for enhanced security:

```ini
[security]
enable_authentication = true
auth_key_file = "/etc/simple-ntpd/keys"
auth_required = true
```

### Rate Limiting

Protect against abuse with rate limiting:

```ini
[security]
enable_rate_limiting = true
max_queries_per_minute = 1000
rate_limit_burst = 100
rate_limit_window = 60
```

### Access Control

Restrict access to specific networks:

```ini
[security]
allow_query_from = [
    "10.0.0.0/8",
    "172.16.0.0/12",
    "192.168.0.0/16"
]
deny_query_from = [
    "0.0.0.0/0"
]
```

### IPv6 Support

Enable IPv6 support for modern networks:

```ini
[network]
enable_ipv6 = true
ipv6_listen_address = "::"
ipv6_listen_port = 123
```

### High Availability

Configure multiple instances for high availability:

```ini
[ntp_server]
upstream_servers = [
    "ntp1.example.com",
    "ntp2.example.com",
    "ntp3.example.com"
]
failover_enabled = true
failover_timeout = 30
```

## Troubleshooting

### Common Issues

#### Service Won't Start
```bash
# Check logs
sudo journalctl -u simple-ntpd -n 50

# Check configuration
sudo simple-ntpd --test-config

# Check permissions
ls -la /var/log/simple-ntpd/
ls -la /etc/simple-ntpd/
```

#### Port Already in Use
```bash
# Check what's using port 123
sudo lsof -i :123
sudo netstat -ulnp | grep :123

# Stop conflicting service
sudo systemctl stop ntp
```

#### Permission Issues
```bash
# Fix ownership
sudo chown -R ntp:ntp /var/log/simple-ntpd
sudo chown -R ntp:ntp /var/lib/simple-ntpd

# Fix permissions
sudo chmod 755 /var/log/simple-ntpd
sudo chmod 644 /etc/simple-ntpd/simple-ntpd.conf
```

### Debug Mode

Run in debug mode for detailed troubleshooting:

```bash
# Run with debug logging
sudo simple-ntpd --verbose --foreground --config /etc/simple-ntpd/simple-ntpd.conf

# Run with specific log level
sudo simple-ntpd --log-level DEBUG --foreground
```

### Performance Issues

#### High CPU Usage
```bash
# Check worker thread count
grep worker_threads /etc/simple-ntpd/simple-ntpd.conf

# Monitor system resources
htop
iotop

# Check for resource limits
ulimit -a
```

#### High Memory Usage
```bash
# Check memory usage
free -h
ps aux | grep simple-ntpd

# Check for memory leaks
valgrind --tool=memcheck simple-ntpd
```

## Best Practices

### Security

1. **Run as non-root user**: Always run the service as a dedicated user
2. **Restrict network access**: Use firewall rules to limit access
3. **Enable rate limiting**: Protect against abuse and DDoS attacks
4. **Regular updates**: Keep the software updated with security patches
5. **Monitor access**: Log and monitor all NTP queries

### Performance

1. **Optimize worker threads**: Match thread count to CPU cores
2. **Tune buffer sizes**: Adjust network buffer sizes for your workload
3. **Monitor resource usage**: Track CPU, memory, and network usage
4. **Use appropriate log levels**: Use INFO in production, DEBUG for troubleshooting
5. **Regular maintenance**: Clean up old logs and temporary files

### Reliability

1. **Multiple upstream servers**: Use at least 3 upstream NTP servers
2. **Monitor synchronization**: Check NTP synchronization status regularly
3. **Backup configuration**: Keep backup copies of configuration files
4. **Test changes**: Always test configuration changes in a safe environment
5. **Documentation**: Document your configuration and procedures

### Monitoring

1. **Health checks**: Implement regular health checks
2. **Metrics collection**: Collect and analyze performance metrics
3. **Alerting**: Set up alerts for critical issues
4. **Log analysis**: Regularly review and analyze logs
5. **Capacity planning**: Monitor trends and plan for growth

## Support

### Getting Help

1. **Check this manual**: Many issues are covered in the troubleshooting section
2. **Review logs**: Check service logs for error messages
3. **Verify configuration**: Use the configuration validation tools
4. **Check system resources**: Ensure adequate system resources
5. **Community support**: Ask questions in the project community

### Reporting Issues

When reporting issues, include:

- Operating system and version
- Simple NTP Daemon version
- Configuration file (sanitized)
- Error messages and logs
- Steps to reproduce the issue
- Expected vs. actual behavior

### Resources

- **Project Repository**: [GitHub Repository](https://github.com/your-repo/simple-ntpd)
- **Issue Tracker**: [GitHub Issues](https://github.com/your-repo/simple-ntpd/issues)
- **Documentation**: [Project Documentation](https://github.com/your-repo/simple-ntpd/docs)
- **Community**: [Discussions](https://github.com/your-repo/simple-ntpd/discussions)

## Conclusion

Simple NTP Daemon provides a robust, secure, and high-performance NTP server solution. By following this manual, you can effectively deploy, configure, and maintain your NTP infrastructure.

Remember to:
- Start with simple configurations and add complexity gradually
- Test all changes in a safe environment
- Monitor your service regularly
- Keep security in mind at all times
- Document your setup and procedures

For additional help and advanced topics, refer to the other documentation sections and community resources.
