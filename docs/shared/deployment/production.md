# Production Deployment Guide

This guide provides comprehensive instructions for deploying Simple NTP Daemon in production environments with enterprise-grade reliability, security, and performance.

## Production Requirements

### Performance Requirements

**Throughput:**
- **Small Environment**: 100-1,000 requests/second
- **Medium Environment**: 1,000-10,000 requests/second
- **Large Environment**: 10,000+ requests/second

**Response Time:**
- **Target**: < 10ms for 95th percentile
- **Acceptable**: < 50ms for 99th percentile
- **Maximum**: < 100ms for 99.9th percentile

**Availability:**
- **Target**: 99.9% uptime (8.76 hours downtime/year)
- **High Availability**: 99.99% uptime (52.6 minutes downtime/year)
- **Critical**: 99.999% uptime (5.26 minutes downtime/year)

### Security Requirements

**Access Control:**
- Network-level access restrictions
- Application-level authentication
- Rate limiting and DDoS protection
- Audit logging and monitoring

**Data Protection:**
- Encrypted communication channels
- Secure key management
- Configuration file protection
- Log data sanitization

**Compliance:**
- Industry security standards
- Regulatory requirements
- Audit trail maintenance
- Security incident response

## Production Architecture

### 1. High Availability Setup

**Primary Architecture:**
```
                    Global Load Balancer
                         │
        ┌────────────────┼────────────────┐
        │                │                │
   Data Center 1    Data Center 2    Data Center 3
        │                │                │
   ┌────┴────┐      ┌────┴────┐      ┌────┴────┐
   │NTP Pool │      │NTP Pool │      │NTP Pool │
   │(3 nodes)│      │(3 nodes)│      │(3 nodes)│
   └─────────┘      └─────────┘      └─────────┘
        │                │                │
   ┌────┴────┐      ┌────┴────┐      ┌────┴────┐
   │Upstream │      │Upstream │      │Upstream │
   │NTP (S1) │      │NTP (S1) │      │NTP (S1) │
   └─────────┘      └─────────┘      └─────────┘
```

**Load Balancer Configuration:**
```yaml
# HAProxy configuration example
global
    maxconn 50000
    log /dev/log local0
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin
    stats timeout 30s
    user haproxy
    group haproxy
    daemon

defaults
    log global
    mode tcp
    option tcplog
    option dontlognull
    timeout connect 5000
    timeout client 50000
    timeout server 50000

frontend ntp_frontend
    bind *:123
    mode udp
    default_backend ntp_backend

backend ntp_backend
    mode udp
    balance roundrobin
    option httpchk
    server ntp1 10.0.1.10:123 check
    server ntp2 10.0.1.11:123 check
    server ntp3 10.0.1.12:123 check
    server ntp4 10.0.2.10:123 check
    server ntp5 10.0.2.11:123 check
    server ntp6 10.0.2.12:123 check
```

### 2. Geographic Distribution

**Multi-Region Setup:**
```yaml
# AWS Route 53 configuration example
apiVersion: v1
kind: Service
metadata:
  name: simple-ntpd-service
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
spec:
  type: LoadBalancer
  ports:
  - port: 123
    targetPort: 123
    protocol: UDP
  selector:
    app: simple-ntpd
```

**DNS Configuration:**
```bash
# Primary NTP servers
ntp1.example.com.     IN  A   10.0.1.10
ntp2.example.com.     IN  A   10.0.1.11
ntp3.example.com.     IN  A   10.0.1.12

# Secondary NTP servers
ntp4.example.com.     IN  A   10.0.2.10
ntp5.example.com.     IN  A   10.0.2.11
ntp6.example.com.     IN  A   10.0.2.12

# Load balancer
ntp.example.com.      IN  A   10.0.1.100
ntp.example.com.      IN  A   10.0.2.100
```

## Production Configuration

### 1. Network Configuration

**High-Performance Network:**
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
enable_ipv6 = true
max_clients = 10000
bind_interface = ""

# Performance tuning
receive_buffer_size = 26214400
send_buffer_size = 26214400
max_connections_per_second = 1000

# Network optimization
enable_tcp_nodelay = true
enable_keepalive = true
keepalive_time = 300
keepalive_probes = 3
keepalive_interval = 10
```

### 2. NTP Server Configuration

**Production NTP Settings:**
```ini
[ntp_server]
stratum = 2
reference_clock = "LOCAL"
reference_id = "PROD"

# Multiple upstream servers for redundancy
upstream_servers = [
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com",
    "time.cloudflare.com",
    "time.windows.com"
]

# Synchronization settings
sync_interval = 60
max_drift = 500
min_poll = 4
max_poll = 17

# Authentication (if required)
enable_authentication = false
auth_key_file = "/etc/simple-ntpd/keys"
```

### 3. Security Configuration

**Production Security:**
```ini
[security]
enable_authentication = false
allow_query_from = [
    "10.0.0.0/8",
    "172.16.0.0/12",
    "192.168.0.0/16"
]
deny_query_from = []

# Rate limiting
enable_rate_limiting = true
max_queries_per_minute = 1000
rate_limit_burst = 100
rate_limit_window = 60

# Access control
enable_dns_validation = true
enable_geoip_restrictions = false
allowed_countries = ["US", "CA", "GB", "DE", "FR"]

# DDoS protection
enable_ddos_protection = true
max_connections_per_ip = 100
connection_timeout = 30
```

### 4. Performance Configuration

**Production Performance:**
```ini
[performance]
worker_threads = 8
max_packet_size = 2048
connection_timeout = 30
idle_timeout = 300

# Connection pooling
enable_connection_pooling = true
max_pool_size = 1000
pool_timeout = 60

# Memory management
max_memory_usage = "1GB"
enable_memory_pooling = true
garbage_collection_interval = 300

# Thread optimization
enable_thread_affinity = true
cpu_affinity = [0, 1, 2, 3, 4, 5, 6, 7]
```

### 5. Logging Configuration

**Production Logging:**
```ini
[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
max_file_size = "100MB"
max_files = 10
log_format = "json"

# Performance logging
log_performance_metrics = true
log_client_queries = false
log_sync_events = true

# Syslog integration
enable_syslog = true
syslog_facility = "daemon"
syslog_tag = "simple-ntpd"

# Structured logging
enable_structured_logging = true
log_timestamp_format = "ISO8601"
log_include_hostname = true
```

### 6. Monitoring Configuration

**Production Monitoring:**
```ini
[monitoring]
enable_metrics = true
metrics_port = 8080
metrics_address = "127.0.0.1"

# Health checks
enable_health_check = true
health_check_interval = 30
health_check_timeout = 10

# SNMP integration
enable_snmp = true
snmp_community = "your-community"
snmp_port = 161
snmp_location = "Data Center 1"
snmp_contact = "ntp-admin@example.com"

# Statistics
enable_statistics = true
stats_retention_days = 90
stats_export_format = "json"
stats_export_interval = 300
```

## Deployment Steps

### 1. Environment Preparation

**System Requirements:**
```bash
# Check system resources
free -h
nproc
df -h

# Verify network configuration
ip addr show
ip route show
ping -c 3 8.8.8.8

# Check firewall status
sudo ufw status
sudo firewall-cmd --list-all
```

**Dependencies Installation:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake libc6-dev libssl-dev

# CentOS/RHEL
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake openssl-devel

# Additional tools
sudo apt install -y htop iotop nethogs
```

### 2. Service Installation

**Automated Installation:**
```bash
# Clone repository
git clone https://github.com/your-repo/simple-ntpd.git
cd simple-ntpd

# Production installation
sudo ./scripts/install.sh --production --high-availability
```

**Manual Installation:**
```bash
# Build and install
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_PRODUCTION=ON ..
make -j$(nproc)
sudo make install

# Create service user
sudo groupadd -r ntp
sudo useradd -r -g ntp -s /bin/false -d /var/lib/simple-ntpd ntp

# Create directories
sudo mkdir -p /etc/simple-ntpd /var/log/simple-ntpd /var/lib/simple-ntpd /var/run/simple-ntpd

# Set permissions
sudo chown -R ntp:ntp /var/log/simple-ntpd /var/lib/simple-ntpd /var/run/simple-ntpd
sudo chmod 755 /var/log/simple-ntpd /var/lib/simple-ntpd /var/run/simple-ntpd
```

### 3. Configuration Deployment

**Production Configuration:**
```bash
# Copy production configuration
sudo cp deployment/configs/production.conf /etc/simple-ntpd/simple-ntpd.conf

# Customize configuration
sudo nano /etc/simple-ntpd/simple-ntpd.conf

# Set permissions
sudo chown ntp:ntp /etc/simple-ntpd/simple-ntpd.conf
sudo chmod 644 /etc/simple-ntpd/simple-ntpd.conf

# Validate configuration
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf
```

**Environment-Specific Configuration:**
```bash
# Create environment-specific configs
sudo cp /etc/simple-ntpd/simple-ntpd.conf /etc/simple-ntpd/simple-ntpd.conf.prod
sudo cp /etc/simple-ntpd/simple-ntpd.conf /etc/simple-ntpd/simple-ntpd.conf.staging

# Environment variables
export SIMPLE_NTPD_ENVIRONMENT="production"
export SIMPLE_NTPD_DATACENTER="dc1"
export SIMPLE_NTPD_REGION="us-east-1"
```

### 4. Service Configuration

**Systemd Service:**
```bash
# Copy service files
sudo cp deployment/systemd/*.service /etc/systemd/system/
sudo cp deployment/systemd/*.socket /etc/systemd/system/

# Customize service file
sudo nano /etc/systemd/system/simple-ntpd.service

# Reload systemd
sudo systemctl daemon-reload

# Enable services
sudo systemctl enable simple-ntpd.socket
sudo systemctl enable simple-ntpd.service
```

**Service File Customization:**
```ini
[Unit]
Description=Simple NTP Daemon
After=network.target
Wants=network.target

[Service]
Type=notify
User=ntp
Group=ntp
ExecStart=/usr/local/bin/simple-ntpd --config /etc/simple-ntpd/simple-ntpd.conf
ExecReload=/bin/kill -HUP $MAINPID
Restart=always
RestartSec=5
LimitNOFILE=65536
LimitNPROC=4096
PrivateTmp=true
ProtectSystem=strict
ReadWritePaths=/var/log/simple-ntpd /var/lib/simple-ntpd /var/run/simple-ntpd

[Install]
WantedBy=multi-user.target
```

### 5. Security Hardening

**Firewall Configuration:**
```bash
# UFW (Ubuntu/Debian)
sudo ufw allow from 10.0.0.0/8 to any port 123 proto udp
sudo ufw allow from 172.16.0.0/12 to any port 123 proto udp
sudo ufw allow from 192.168.0.0/16 to any port 123 proto udp

# firewalld (RHEL/CentOS)
sudo firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="10.0.0.0/8" port port="123" protocol="udp" accept'
sudo firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="172.16.0.0/12" port port="123" protocol="udp" accept'
sudo firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="192.168.0.0/16" port port="123" protocol="udp" accept'
sudo firewall-cmd --reload
```

**System Hardening:**
```bash
# Disable unnecessary services
sudo systemctl disable avahi-daemon
sudo systemctl disable cups
sudo systemctl disable bluetooth

# Secure shared memory
echo 'tmpfs     /run/shm     tmpfs     defaults,noexec,nosuid     0     0' | sudo tee -a /etc/fstab

# Secure kernel parameters
echo 'net.ipv4.tcp_syncookies = 1' | sudo tee -a /etc/sysctl.conf
echo 'net.ipv4.tcp_max_syn_backlog = 2048' | sudo tee -a /etc/sysctl.conf
echo 'net.core.somaxconn = 65536' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

### 6. Monitoring Setup

**Prometheus Configuration:**
```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'simple-ntpd'
    static_configs:
      - targets: ['localhost:8080']
        labels:
          service: 'simple-ntpd'
          environment: 'production'
          datacenter: 'dc1'
    scrape_interval: 10s
    metrics_path: '/metrics'
```

**Grafana Dashboard:**
```bash
# Import production dashboard
wget https://raw.githubusercontent.com/your-repo/simple-ntpd/main/deployment/examples/monitoring/grafana-production-dashboard.json

# Import in Grafana
# 1. Open Grafana
# 2. Click + icon → Import
# 3. Upload JSON file
# 4. Select Prometheus data source
# 5. Import
```

**Alerting Rules:**
```yaml
# simple-ntpd-rules.yml
groups:
  - name: production-ntp
    rules:
      - alert: NTPServiceDown
        expr: up{job="simple-ntpd"} == 0
        for: 1m
        labels:
          severity: critical
          environment: production
        annotations:
          summary: "NTP service is down in production"
          description: "Simple NTP Daemon service has been down for more than 1 minute"
```

## Performance Tuning

### 1. System Optimization

**Kernel Parameters:**
```bash
# Network optimization
echo 'net.core.rmem_max = 26214400' | sudo tee -a /etc/sysctl.conf
echo 'net.core.wmem_max = 26214400' | sudo tee -a /etc/sysctl.conf
echo 'net.core.rmem_default = 26214400' | sudo tee -a /etc/sysctl.conf
echo 'net.core.wmem_default = 26214400' | sudo tee -a /etc/sysctl.conf

# UDP optimization
echo 'net.ipv4.udp_rmem_min = 4096' | sudo tee -a /etc/sysctl.conf
echo 'net.ipv4.udp_wmem_min = 4096' | sudo tee -a /etc/sysctl.conf

# Apply changes
sudo sysctl -p
```

**Process Limits:**
```bash
# Increase file descriptor limits
echo 'ntp soft nofile 65536' | sudo tee -a /etc/security/limits.conf
echo 'ntp hard nofile 65536' | sudo tee -a /etc/security/limits.conf
echo 'ntp soft nproc 4096' | sudo tee -a /etc/security/limits.conf
echo 'ntp hard nproc 4096' | sudo tee -a /etc/security/limits.conf
```

### 2. Application Optimization

**Configuration Tuning:**
```ini
[performance]
# Optimize for your CPU
worker_threads = 8  # Match CPU cores
enable_thread_affinity = true
cpu_affinity = [0, 1, 2, 3, 4, 5, 6, 7]

# Memory optimization
max_memory_usage = "2GB"
enable_memory_pooling = true
memory_pool_size = "1GB"

# Network optimization
max_packet_size = 4096
connection_timeout = 15
idle_timeout = 180
```

**Load Testing:**
```bash
# Install load testing tools
sudo apt install -y apache2-utils siege

# Test with multiple clients
for i in {1..100}; do
  ntpdate -q localhost &
done
wait

# Monitor performance
htop
iotop
nethogs
```

## Backup and Recovery

### 1. Configuration Backup

**Automated Backup:**
```bash
# Create backup script
sudo tee /usr/local/bin/backup-ntpd << 'EOF'
#!/bin/bash
BACKUP_DIR="/var/backup/simple-ntpd"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR
cp /etc/simple-ntpd/simple-ntpd.conf $BACKUP_DIR/simple-ntpd.conf.$DATE
cp -r /etc/simple-ntpd $BACKUP_DIR/config.$DATE

# Keep only last 30 days
find $BACKUP_DIR -name "*.conf.*" -mtime +30 -delete
find $BACKUP_DIR -name "config.*" -mtime +30 -exec rm -rf {} \;
EOF

sudo chmod +x /usr/local/bin/backup-ntpd

# Add to crontab
echo "0 2 * * * /usr/local/bin/backup-ntpd" | sudo crontab -
```

**Recovery Procedures:**
```bash
# Restore configuration
sudo cp /var/backup/simple-ntpd/simple-ntpd.conf.20240101_020000 /etc/simple-ntpd/simple-ntpd.conf

# Restart service
sudo systemctl restart simple-ntpd

# Verify recovery
sudo systemctl status simple-ntpd
ntpdate -q localhost
```

### 2. Disaster Recovery

**Recovery Plan:**
```bash
# 1. Stop service
sudo systemctl stop simple-ntpd

# 2. Restore from backup
sudo cp /var/backup/simple-ntpd/simple-ntpd.conf.latest /etc/simple-ntpd/simple-ntpd.conf

# 3. Verify configuration
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf

# 4. Start service
sudo systemctl start simple-ntpd

# 5. Verify functionality
sudo systemctl status simple-ntpd
ntpdate -q localhost
```

## Maintenance Procedures

### 1. Regular Maintenance

**Daily Tasks:**
```bash
# Check service status
sudo systemctl status simple-ntpd

# Check logs for errors
sudo journalctl -u simple-ntpd --since "1 day ago" | grep ERROR

# Check resource usage
htop
df -h
```

**Weekly Tasks:**
```bash
# Check configuration
sudo simple-ntpd --test-config

# Review logs
sudo journalctl -u simple-ntpd --since "1 week ago" | grep -E "(ERROR|WARN)"

# Check performance metrics
curl http://localhost:8080/metrics | grep ntp_
```

**Monthly Tasks:**
```bash
# Update system packages
sudo apt update && sudo apt upgrade

# Review security updates
sudo apt list --upgradable | grep security

# Check backup integrity
sudo /usr/local/bin/backup-ntpd --verify
```

### 2. Update Procedures

**Zero-Downtime Updates:**
```bash
# 1. Deploy new version to standby server
sudo systemctl stop simple-ntpd
sudo make install
sudo systemctl start simple-ntpd

# 2. Verify new version
sudo simple-ntpd --version
ntpdate -q localhost

# 3. Update load balancer configuration
# 4. Monitor for issues
# 5. Update remaining servers
```

## Troubleshooting

### 1. Common Production Issues

**High Response Time:**
```bash
# Check system resources
htop
iostat -x 1
netstat -i

# Check network configuration
ip addr show
ip route show
ping -c 10 8.8.8.8

# Check NTP statistics
ntpq -p
ntpstat
```

**Service Outages:**
```bash
# Check service status
sudo systemctl status simple-ntpd

# Check logs
sudo journalctl -u simple-ntpd -n 100

# Check configuration
sudo simple-ntpd --test-config

# Check dependencies
sudo systemctl status network-online.target
```

### 2. Performance Issues

**High CPU Usage:**
```bash
# Profile the application
sudo perf record -g -p $(pgrep simple-ntpd)
sudo perf report

# Check thread usage
ps -eLf | grep simple-ntpd | wc -l

# Monitor system calls
sudo strace -p $(pgrep simple-ntpd) -c
```

**Memory Issues:**
```bash
# Check memory usage
free -h
ps aux | grep simple-ntpd

# Check for memory leaks
sudo valgrind --tool=memcheck --leak-check=full simple-ntpd

# Monitor memory allocation
sudo strace -p $(pgrep simple-ntpd) -e trace=memory
```

## Conclusion

Production deployment of Simple NTP Daemon requires careful planning, proper configuration, and ongoing maintenance. By following this guide, you can create a robust, secure, and high-performance NTP infrastructure that meets enterprise requirements.

Key success factors:
- **Proper Planning**: Understand requirements and design appropriate architecture
- **Security First**: Implement comprehensive security measures
- **Performance Tuning**: Optimize for your specific workload
- **Monitoring**: Implement comprehensive monitoring and alerting
- **Maintenance**: Establish regular maintenance procedures
- **Documentation**: Maintain up-to-date documentation and procedures

For additional guidance, refer to the troubleshooting guide and community resources.
