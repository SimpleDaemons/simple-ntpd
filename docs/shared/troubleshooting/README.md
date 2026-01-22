# Troubleshooting Guide

This guide helps you diagnose and resolve common issues with the Simple NTP Daemon. Follow the troubleshooting steps systematically to identify and fix problems.

## 🚨 Quick Problem Resolution

### Server Won't Start
- [Check permissions](#permission-issues)
- [Verify port availability](#port-conflicts)
- [Check configuration syntax](#configuration-errors)

### Clients Can't Connect
- [Verify network connectivity](#network-connectivity)
- [Check firewall settings](#firewall-issues)
- [Validate client configuration](#client-configuration)

### Time Synchronization Issues
- [Check upstream server connectivity](#upstream-server-issues)
- [Verify stratum configuration](#stratum-issues)
- [Monitor synchronization status](#synchronization-monitoring)

## 🔍 Diagnostic Commands

### Basic Health Checks

```bash
# Check if server is running
ps aux | grep simple-ntpd

# Check listening ports
sudo netstat -tulpn | grep :123

# Check server status
sudo systemctl status simple-ntpd

# View recent logs
tail -f /var/log/simple-ntpd/ntpd.log
```

### NTP Client Testing

```bash
# Test with ntpdate
ntpdate -q localhost

# Test with chrony
chronyc sources

# Test with ntpq
ntpq -p localhost

# Simple UDP test
echo "test" | nc -u localhost 123
```

### Network Diagnostics

```bash
# Check network interfaces
ip addr show

# Test connectivity to upstream servers
ping time.nist.gov
nslookup pool.ntp.org

# Check routing
traceroute time.google.com
```

## 🚨 Common Issues and Solutions

### Permission Issues

#### Problem: "Permission denied" when starting server

**Symptoms:**
- Server fails to start with permission errors
- Cannot bind to port 123
- Access denied to log files

**Causes:**
- NTP port 123 requires root privileges
- Insufficient file permissions
- User/group configuration issues

**Solutions:**

```bash
# Run with sudo (temporary solution)
sudo ./simple-ntpd -c config.conf

# Create dedicated user and group
sudo useradd -r -s /bin/false ntp
sudo groupadd ntp

# Set proper ownership
sudo chown -R ntp:ntp /var/log/simple-ntpd
sudo chown -R ntp:ntp /etc/simple-ntpd

# Use capabilities (Linux only)
sudo setcap 'cap_net_bind_service=+ep' /usr/local/bin/simple-ntpd
```

### Port Conflicts

#### Problem: "Address already in use" or port conflicts

**Symptoms:**
- Server fails to start
- Port 123 already in use
- Network binding errors

**Causes:**
- Another NTP service running
- System time synchronization service
- Previous instance not properly stopped

**Solutions:**

```bash
# Check what's using port 123
sudo netstat -tulpn | grep :123
sudo lsof -i :123

# Stop conflicting services
sudo systemctl stop systemd-timesyncd
sudo systemctl stop ntp
sudo systemctl stop chronyd

# Kill existing processes
sudo pkill -f simple-ntpd

# Use different port (temporary)
./simple-ntpd -c config.conf --port 1234
```

### Configuration Errors

#### Problem: Configuration file syntax errors

**Symptoms:**
- Server fails to start
- Configuration parsing errors
- Invalid parameter values

**Causes:**
- Syntax errors in config file
- Invalid parameter values
- Missing required parameters

**Solutions:**

```bash
# Validate configuration
./simple-ntpd --validate-config config.conf

# Check configuration syntax
./simple-ntpd --check-config config.conf

# Test configuration without starting
./simple-ntpd --dry-run -c config.conf

# Use example configuration as template
cp config/examples/simple/simple-ntpd.conf.example config.conf
```

### Network Connectivity Issues

#### Problem: Clients cannot connect to server

**Symptoms:**
- Clients timeout when querying server
- Network unreachable errors
- Connection refused messages

**Causes:**
- Firewall blocking connections
- Network interface binding issues
- Routing problems

**Solutions:**

```bash
# Check firewall status
sudo ufw status
sudo iptables -L
sudo firewall-cmd --list-all

# Allow NTP through firewall
sudo ufw allow 123/udp
sudo iptables -A INPUT -p udp --dport 123 -j ACCEPT

# Check network interface binding
ip addr show
netstat -i

# Test local connectivity
telnet localhost 123
nc -u localhost 123
```

### Upstream Server Issues

#### Problem: Server cannot synchronize with upstream servers

**Symptoms:**
- High stratum values
- Sync errors in logs
- Time drift issues

**Causes:**
- Upstream servers unreachable
- Network connectivity issues
- DNS resolution problems

**Solutions:**

```bash
# Test upstream server connectivity
ping pool.ntp.org
nslookup time.nist.gov

# Check DNS resolution
dig pool.ntp.org
nslookup pool.ntp.org

# Test specific upstream servers
ntpdate -q time.nist.gov
ntpdate -q time.google.com

# Use IP addresses instead of hostnames
upstream_servers = 129.6.15.28, 8.8.8.8
```

### Logging Issues

#### Problem: No logs or excessive logging

**Symptoms:**
- Missing log files
- Excessive log output
- Performance degradation

**Causes:**
- Incorrect log configuration
- File permission issues
- Log rotation problems

**Solutions:**

```bash
# Check log file permissions
ls -la /var/log/simple-ntpd/
sudo chown -R ntp:ntp /var/log/simple-ntpd

# Verify log configuration
grep -i log config.conf

# Check disk space
df -h /var/log

# Test logging manually
./simple-ntpd --log-level DEBUG --log-file /tmp/test.log
```

## 🔧 Advanced Troubleshooting

### Performance Issues

#### Problem: High latency or low throughput

**Symptoms:**
- Slow response times
- Connection timeouts
- High CPU usage

**Causes:**
- Insufficient resources
- Network congestion
- Configuration bottlenecks

**Solutions:**

```bash
# Monitor system resources
htop
iotop
nethogs

# Check network performance
iperf3 -c localhost
netperf -H localhost

# Optimize configuration
worker_threads = 8
max_connections = 10000
enable_connection_pooling = true
```

### Memory Issues

#### Problem: High memory usage or memory leaks

**Symptoms:**
- Increasing memory consumption
- Out of memory errors
- Performance degradation

**Causes:**
- Memory leaks in code
- Excessive caching
- Resource cleanup issues

**Solutions:**

```bash
# Monitor memory usage
free -h
ps aux --sort=-%mem | head

# Check for memory leaks
valgrind --leak-check=full ./simple-ntpd

# Adjust memory settings
max_memory_usage = 512MB
enable_memory_pooling = false
```

### Security Issues

#### Problem: Unauthorized access or security concerns

**Symptoms:**
- Unexpected connections
- Failed authentication
- Security warnings

**Causes:**
- Weak access controls
- Missing authentication
- Network exposure

**Solutions:**

```bash
# Review access controls
grep -i allow config.conf
grep -i deny config.conf

# Enable authentication
enable_authentication = true
authentication_key = secure-key

# Restrict network access
allowed_networks = 192.168.1.0/24
denied_networks = 0.0.0.0/0
```

## 📊 Monitoring and Diagnostics

### Health Check Endpoints

```bash
# Check server health
curl http://localhost:8080/health

# Get metrics
curl http://localhost:9090/metrics

# Check configuration
curl http://localhost:8080/config
```

### Log Analysis

```bash
# Search for errors
grep -i error /var/log/simple-ntpd/ntpd.log

# Monitor real-time logs
tail -f /var/log/simple-ntpd/ntpd.log | grep -E "(ERROR|WARN|FATAL)"

# Analyze log patterns
awk '/ERROR/ {print $1, $2, $NF}' /var/log/simple-ntpd/ntpd.log
```

### Performance Metrics

```bash
# Check response times
ntpq -c "rv" localhost

# Monitor connections
netstat -an | grep :123 | wc -l

# Check system resources
vmstat 1 10
iostat 1 10
```

## 🆘 Getting Help

### Before Asking for Help

1. **Check the logs**: Look for error messages and warnings
2. **Verify configuration**: Ensure config file syntax is correct
3. **Test connectivity**: Verify network and upstream server access
4. **Check permissions**: Ensure proper file and port access
5. **Review documentation**: Check this guide and other docs

### When to Seek Help

- **Critical errors**: Server won't start or crashes
- **Security issues**: Unauthorized access or vulnerabilities
- **Performance problems**: Unacceptable latency or throughput
- **Configuration issues**: Complex setup requirements

### How to Report Issues

1. **Use the issue template**: Follow the GitHub issue template
2. **Include logs**: Provide relevant log excerpts
3. **Describe environment**: OS, version, configuration
4. **Reproduce steps**: Clear steps to reproduce the issue
5. **Check existing issues**: Search for similar problems

### Support Channels

- **GitHub Issues**: [Report bugs and issues](https://github.com/blburns/simple-ntpd/issues)
- **GitHub Discussions**: [Ask questions and get help](https://github.com/blburns/simple-ntpd/discussions)
- **Documentation**: [Check the full documentation](../README.md)
- **FAQ**: [Common questions and answers](../support/faq.md)

## 📚 Additional Resources

- [Configuration Guide](../configuration/README.md)
- [Deployment Guide](../deployment/README.md)
- [Performance Tuning](../deployment/performance.md)
- [Security Hardening](../deployment/security.md)
- [API Reference](../developer/api-reference.md)

---

*If you can't find a solution here, please check the [FAQ](../support/faq.md) or open an issue on GitHub.*
