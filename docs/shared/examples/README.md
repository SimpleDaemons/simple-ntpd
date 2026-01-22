# Configuration Examples

This directory contains real-world configuration examples for Simple NTP Daemon across different environments and use cases.

## Example Categories

### 1. **Basic Configurations**
Simple configurations for common deployment scenarios:
- [Simple NTP Server](simple/simple-ntpd.conf.example) - Basic NTP server setup
- [Development Environment](development/development.conf) - Development and testing setup
- [Production Environment](production/production.conf) - Production-ready configuration

### 2. **Advanced Configurations**
Complex configurations for specialized requirements:
- [High Performance](advanced/high-performance.conf) - Optimized for high-throughput environments
- [High Security](advanced/high-security.conf) - Security-hardened configuration
- [Load Balanced](advanced/load-balanced.conf) - Multi-instance load balancing setup

### 3. **Environment-Specific Configurations**
Configurations tailored for specific environments:
- [Docker](docker/docker.conf) - Containerized deployment configuration
- [Kubernetes](kubernetes/k8s.conf) - Kubernetes deployment configuration
- [Cloud](cloud/aws.conf) - Cloud provider specific configurations

## Quick Start Examples

### Basic NTP Server
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123

[ntp_server]
stratum = 2
upstream_servers = ["pool.ntp.org", "time.nist.gov"]

[logging]
level = "INFO"
output = "file"
```

### Development Environment
```ini
[network]
listen_address = 127.0.0.1
listen_port = 123

[ntp_server]
stratum = 3
upstream_servers = ["pool.ntp.org"]

[logging]
level = "DEBUG"
output = "console"

[development]
enable_hot_reload = true
enable_test_endpoints = true
```

### Production Environment
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
max_clients = 10000

[ntp_server]
stratum = 2
upstream_servers = [
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
]

[logging]
level = "INFO"
output = "file"
log_format = "json"

[security]
enable_rate_limiting = true
max_queries_per_minute = 1000

[performance]
worker_threads = 8
max_packet_size = 2048
```

## Configuration Templates

### 1. **Simple Template**
Use this template for basic NTP server setup:

```ini
[network]
listen_address = 0.0.0.0
listen_port = 123

[ntp_server]
stratum = 2
upstream_servers = ["pool.ntp.org", "time.nist.gov"]

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"

[security]
enable_authentication = false
allow_query_from = ["0.0.0.0/0"]

[performance]
worker_threads = 4
max_packet_size = 1024
```

### 2. **Development Template**
Use this template for development and testing:

```ini
[network]
listen_address = 127.0.0.1
listen_port = 123

[ntp_server]
stratum = 3
upstream_servers = ["pool.ntp.org"]

[logging]
level = "DEBUG"
output = "console"

[development]
enable_hot_reload = true
enable_test_endpoints = true
enable_debug_endpoints = true

[performance]
worker_threads = 2
max_packet_size = 1024
```

### 3. **Production Template**
Use this template for production environments:

```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
max_clients = 10000

[ntp_server]
stratum = 2
upstream_servers = [
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com",
    "time.cloudflare.com"
]

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
log_format = "json"
max_file_size = "100MB"
max_files = 10

[security]
enable_authentication = false
allow_query_from = [
    "10.0.0.0/8",
    "172.16.0.0/12",
    "192.168.0.0/16"
]
enable_rate_limiting = true
max_queries_per_minute = 1000

[performance]
worker_threads = 8
max_packet_size = 2048
connection_timeout = 30
idle_timeout = 300

[monitoring]
enable_metrics = true
metrics_port = 8080
enable_health_check = true
```

## Environment-Specific Examples

### Docker Environment
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123

[ntp_server]
stratum = 2
upstream_servers = ["pool.ntp.org", "time.nist.gov"]

[logging]
level = "INFO"
output = "console"

[performance]
worker_threads = 4
max_packet_size = 1024
```

### Kubernetes Environment
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
max_clients = 5000

[ntp_server]
stratum = 2
upstream_servers = [
    "pool.ntp.org",
    "time.nist.gov"
]

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"

[performance]
worker_threads = 4
max_packet_size = 1024
```

### Cloud Environment (AWS)
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
max_clients = 10000

[ntp_server]
stratum = 2
upstream_servers = [
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
]

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
log_format = "json"

[security]
enable_rate_limiting = true
max_queries_per_minute = 1000

[performance]
worker_threads = 8
max_packet_size = 2048
```

## Advanced Configuration Examples

### High Performance Configuration
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
max_clients = 50000

[ntp_server]
stratum = 2
upstream_servers = [
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
]

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"

[performance]
worker_threads = 16
max_packet_size = 4096
connection_timeout = 15
idle_timeout = 180
enable_connection_pooling = true
max_pool_size = 5000

[monitoring]
enable_metrics = true
metrics_port = 8080
```

### High Security Configuration
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
max_clients = 1000

[ntp_server]
stratum = 2
upstream_servers = ["pool.ntp.org", "time.nist.gov"]

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
log_format = "json"

[security]
enable_authentication = true
auth_key_file = "/etc/simple-ntpd/keys"
allow_query_from = [
    "10.0.0.0/8",
    "172.16.0.0/12"
]
enable_rate_limiting = true
max_queries_per_minute = 500
enable_dns_validation = true
enable_geoip_restrictions = true
allowed_countries = ["US", "CA", "GB"]

[performance]
worker_threads = 4
max_packet_size = 1024
```

## Configuration Best Practices

### 1. **Security Best Practices**
- Use specific IP ranges for `allow_query_from`
- Enable rate limiting in production
- Use authentication for sensitive environments
- Restrict access to trusted networks

### 2. **Performance Best Practices**
- Match `worker_threads` to CPU cores
- Use appropriate `max_packet_size`
- Enable connection pooling for high-load environments
- Monitor resource usage and adjust accordingly

### 3. **Logging Best Practices**
- Use `INFO` level in production
- Use `DEBUG` level only in development
- Implement log rotation
- Use structured logging (JSON) in production

### 4. **Monitoring Best Practices**
- Enable metrics endpoint
- Implement health checks
- Set up alerting for critical metrics
- Monitor performance and resource usage

## Customization Guidelines

### 1. **Network Configuration**
- Set `listen_address` based on your network topology
- Adjust `max_clients` based on expected load
- Configure IPv6 if needed

### 2. **NTP Server Configuration**
- Set appropriate `stratum` level
- Use multiple upstream servers for redundancy
- Configure sync intervals based on requirements

### 3. **Security Configuration**
- Restrict access to necessary networks only
- Implement appropriate rate limiting
- Use authentication if required

### 4. **Performance Configuration**
- Optimize worker threads for your hardware
- Adjust buffer sizes based on network conditions
- Configure timeouts appropriately

## Testing Your Configuration

### 1. **Configuration Validation**
```bash
# Test configuration file
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf

# Validate configuration syntax
sudo simple-ntpd --validate-config --config /etc/simple-ntpd/simple-ntpd.conf
```

### 2. **Functional Testing**
```bash
# Start service with configuration
sudo simple-ntpd start --config /etc/simple-ntpd/simple-ntpd.conf

# Test NTP functionality
ntpdate -q localhost

# Check service status
sudo systemctl status simple-ntpd
```

### 3. **Performance Testing**
```bash
# Load test with multiple clients
for i in {1..100}; do
  ntpdate -q localhost &
done
wait

# Monitor performance
htop
netstat -i
```

## Troubleshooting Configuration Issues

### 1. **Common Configuration Errors**
- Invalid syntax in configuration file
- Missing required sections
- Invalid parameter values
- Permission issues with files

### 2. **Configuration Debugging**
```bash
# Run with verbose logging
sudo simple-ntpd --verbose --foreground --config /etc/simple-ntpd/simple-ntpd.conf

# Check configuration file permissions
ls -la /etc/simple-ntpd/simple-ntpd.conf

# Validate configuration step by step
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf
```

### 3. **Getting Help**
- Check the configuration reference documentation
- Review the troubleshooting guide
- Check community forums and discussions
- Create an issue in the project repository

## Conclusion

These configuration examples provide a starting point for various deployment scenarios. Use them as templates and customize them according to your specific requirements.

Remember to:
- Always test configurations before deploying to production
- Follow security best practices
- Monitor performance and adjust as needed
- Document your customizations
- Keep configurations under version control

For additional help and advanced configurations, refer to the main documentation and community resources.
