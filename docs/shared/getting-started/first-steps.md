# First Steps: Your First NTP Server

This guide walks you through setting up your first NTP server with Simple NTP Daemon in just a few minutes.

## Prerequisites

Before starting, ensure you have:
- A Linux, macOS, or Windows system
- Root or administrator access
- Basic command line knowledge
- Network access to the internet

## Quick Setup (5 Minutes)

### Step 1: Download and Build

```bash
# Clone the repository
git clone https://github.com/your-repo/simple-ntpd.git
cd simple-ntpd

# Build the project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Install
sudo make install
```

### Step 2: Create Basic Configuration

```bash
# Create configuration directory
sudo mkdir -p /etc/simple-ntpd

# Copy example configuration
sudo cp ../config/examples/simple/simple-ntpd.conf.example /etc/simple-ntpd/simple-ntpd.conf

# Edit configuration (optional)
sudo nano /etc/simple-ntpd/simple-ntpd.conf
```

### Step 3: Start the Service

```bash
# Run in foreground to test
sudo simple-ntpd --config /etc/simple-ntpd/simple-ntpd.conf

# Or start as a service
sudo systemctl start simple-ntpd
```

### Step 4: Test Your NTP Server

```bash
# Test from another machine
ntpdate -q your-server-ip

# Or test locally
ntpdate -q localhost
```

## Detailed Setup

### 1. System Preparation

#### Create Service User

```bash
# Create dedicated user for security
sudo groupadd -r ntp
sudo useradd -r -g ntp -s /bin/false -d /var/lib/simple-ntpd ntp

# Create directories
sudo mkdir -p /var/log/simple-ntpd
sudo mkdir -p /var/lib/simple-ntpd
sudo mkdir -p /var/run/simple-ntpd

# Set permissions
sudo chown -R ntp:ntp /var/log/simple-ntpd /var/lib/simple-ntpd /var/run/simple-ntpd
```

#### Configure Firewall

```bash
# Allow NTP traffic (UDP port 123)
sudo ufw allow 123/udp  # Ubuntu/Debian
# OR
sudo firewall-cmd --permanent --add-service=ntp  # RHEL/CentOS
sudo firewall-cmd --reload
```

### 2. Configuration

#### Basic Configuration File

Create `/etc/simple-ntpd/simple-ntpd.conf`:

```ini
[network]
listen_address = 0.0.0.0
listen_port = 123
enable_ipv6 = true

[ntp_server]
stratum = 2
reference_clock = "LOCAL"
upstream_servers = [
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
]
sync_interval = 60

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
max_file_size = "10MB"
max_files = 5

[security]
enable_authentication = false
allow_query_from = ["0.0.0.0/0"]

[performance]
worker_threads = 4
max_packet_size = 1024
```

#### Environment Variables

You can also use environment variables:

```bash
export SIMPLE_NTPD_LISTEN_ADDRESS="0.0.0.0"
export SIMPLE_NTPD_LISTEN_PORT="123"
export SIMPLE_NTPD_STRATUM="2"
export SIMPLE_NTPD_LOG_LEVEL="INFO"
```

### 3. Service Installation

#### Linux (systemd)

```bash
# Copy service files
sudo cp ../deployment/systemd/*.service /etc/systemd/system/
sudo cp ../deployment/systemd/*.socket /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable and start
sudo systemctl enable simple-ntpd.socket
sudo systemctl enable simple-ntpd.service
sudo systemctl start simple-ntpd
```

#### Linux (init.d)

```bash
# Copy init script
sudo cp ../deployment/init.d/simple-ntpd /etc/init.d/
sudo chmod 755 /etc/init.d/simple-ntpd

# Enable service
sudo update-rc.d simple-ntpd defaults  # Debian/Ubuntu
# OR
sudo chkconfig --add simple-ntpd       # RedHat/CentOS
```

#### macOS

```bash
# Install launchd service
sudo cp ../deployment/launchd/com.simpledaemons.simple-ntpd.plist /Library/LaunchDaemons/
sudo chown root:wheel /Library/LaunchDaemons/com.simpledaemons.simple-ntpd.plist

# Load and start
sudo launchctl load /Library/LaunchDaemons/com.simpledaemons.simple-ntpd.plist
sudo launchctl start com.simpledaemons.simple-ntpd
```

#### Windows

```bash
# Install as Windows service (run as Administrator)
cd ../deployment/windows
.\install-service.bat

# Start service
net start simple-ntpd
```

### 4. Verification

#### Check Service Status

```bash
# Linux systemd
sudo systemctl status simple-ntpd

# Linux init.d
sudo /etc/init.d/simple-ntpd status

# macOS
sudo launchctl list | grep simple-ntpd

# Windows
sc query simple-ntpd
```

#### Test NTP Functionality

```bash
# Check if port is listening
sudo netstat -ulnp | grep :123
sudo ss -ulnp | grep :123

# Test NTP query
ntpdate -q localhost

# Check logs
sudo tail -f /var/log/simple-ntpd/simple-ntpd.log
```

#### Verify Time Synchronization

```bash
# Check system time
date

# Check NTP status
ntpq -p

# Monitor NTP statistics
ntpstat
```

## Configuration Examples

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
    "time.cloudflare.com"
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

## Troubleshooting

### Common Issues

1. **Permission Denied**
   ```bash
   sudo chown -R ntp:ntp /var/log/simple-ntpd
   sudo chmod 755 /var/log/simple-ntpd
   ```

2. **Port Already in Use**
   ```bash
   sudo lsof -i :123
   sudo systemctl stop ntp  # Stop conflicting service
   ```

3. **Service Won't Start**
   ```bash
   sudo journalctl -u simple-ntpd -n 50
   sudo simple-ntpd --test-config
   ```

4. **Configuration Errors**
   ```bash
   sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf
   ```

### Debug Mode

```bash
# Run with verbose logging
sudo simple-ntpd --verbose --foreground --config /etc/simple-ntpd/simple-ntpd.conf

# Check configuration
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf
```

## Next Steps

After your NTP server is running:

1. **Configure Monitoring**: Set up health checks and metrics
2. **Security Hardening**: Apply security best practices
3. **Performance Tuning**: Optimize for your workload
4. **Client Configuration**: Configure clients to use your server
5. **Backup Strategy**: Implement configuration backup

## Client Configuration

### Linux Clients

```bash
# Add to /etc/ntp.conf
server your-ntp-server-ip iburst

# Or use systemd-timesyncd
sudo timedatectl set-ntp true
```

### Windows Clients

```cmd
# Set NTP server
w32tm /config /manualpeerlist:"your-ntp-server-ip" /syncfromflags:manual
w32tm /config /update
w32tm /resync
```

### Network Devices

Most network devices support NTP configuration through their web interface or CLI:

```
ntp server your-ntp-server-ip
```

## Support

If you encounter issues:

1. Check the [Troubleshooting Guide](../troubleshooting/README.md)
2. Review system logs and service status
3. Verify network connectivity
4. Check firewall configuration
5. Create an issue in the project repository

## Success!

You now have a working NTP server! Your server is providing accurate time synchronization to your network and can serve as a reliable time source for other systems.
