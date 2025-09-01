# Simple NTP Daemon Deployment Guide

This directory contains all the necessary files and scripts for deploying the Simple NTP Daemon on various systems.

## Directory Structure

```
deployment/
├── README.md                    # This file
├── DEPLOYMENT_CHECKLIST.md      # Comprehensive deployment checklist
├── QUICK_START_GUIDES.md        # Platform-specific quick start guides
├── systemd/                     # Systemd service files
│   ├── simple-ntpd.service
│   ├── simple-ntpd.socket
│   └── simple-ntpd.target
├── init.d/                      # Traditional init.d scripts
│   └── simple-ntpd
├── launchd/                     # macOS launchd service
│   └── com.simpledaemons.simple-ntpd.plist
├── windows/                     # Windows service installation
│   ├── install-service.bat
│   ├── install-service.ps1
│   └── simple-ntpd-service.cpp
├── configs/                     # Configuration templates
│   ├── production.conf          # Production configuration
│   └── development.conf         # Development configuration
├── templates/                   # System configuration templates
│   ├── logrotate.conf          # Log rotation configuration
│   └── firewall.conf           # Firewall configuration templates
├── examples/                    # Deployment examples
│   ├── docker/                 # Docker deployment
│   │   ├── docker-compose.yml
│   │   └── README.md
│   ├── kubernetes/             # Kubernetes deployment
│   │   ├── deployment.yaml
│   │   ├── service.yaml
│   │   ├── configmap.yaml
│   │   ├── namespace.yaml
│   │   ├── serviceaccount.yaml
│   │   └── README.md
│   └── monitoring/             # Monitoring and observability
│       ├── prometheus.yml      # Prometheus configuration
│       ├── alerting-rules.yml  # Alerting rules
│       └── README.md           # Monitoring guide
└── scripts/                    # Deployment scripts
    └── install.sh
```

## Deployment Methods

### 1. Quick Start Guides

For platform-specific deployment instructions, see:
- **Linux**: [Linux Quick Start Guide](QUICK_START_GUIDES.md#linux-quick-start-systemd)
- **Docker**: [Docker Quick Start Guide](QUICK_START_GUIDES.md#docker-quick-start)
- **Kubernetes**: [Kubernetes Quick Start Guide](QUICK_START_GUIDES.md#kubernetes-quick-start)
- **macOS**: [macOS Quick Start Guide](QUICK_START_GUIDES.md#macos-quick-start)
- **Windows**: [Windows Quick Start Guide](QUICK_START_GUIDES.md#windows-quick-start)

### 2. Configuration Templates

Ready-to-use configuration files:
- **Production**: [Production Configuration](configs/production.conf) - Optimized for high-performance production environments
- **Development**: [Development Configuration](configs/development.conf) - Suitable for development and testing

### 3. System Integration Templates

- **Log Rotation**: [Logrotate Configuration](templates/logrotate.conf) - Automatic log management
- **Firewall Rules**: [Firewall Configuration](templates/firewall.conf) - Platform-specific firewall setup

### 4. Container Deployment

- **Docker**: Complete Docker Compose setup with [Docker Guide](examples/docker/README.md)
- **Kubernetes**: Production-ready Kubernetes manifests with [Kubernetes Guide](examples/kubernetes/README.md)

### 5. Monitoring and Observability

- **Prometheus**: [Prometheus Configuration](examples/monitoring/prometheus.yml) - Metrics collection
- **Alerting**: [Alerting Rules](examples/monitoring/alerting-rules.yml) - Comprehensive alerting
- **Monitoring Guide**: [Complete Monitoring Guide](examples/monitoring/README.md) - Setup and best practices

### 6. Deployment Checklist

Use the [Deployment Checklist](DEPLOYMENT_CHECKLIST.md) to ensure complete and secure deployment.

### 7. Automated Installation Script

The `scripts/install.sh` script provides a comprehensive, automated installation process:

```bash
# Full installation (recommended)
sudo ./deployment/scripts/install.sh

# Installation without building (for pre-built packages)
sudo ./deployment/scripts/install.sh --skip-build

# Installation without systemd (for older systems)
sudo ./deployment/scripts/install.sh --skip-systemd

# Show all options
./deployment/scripts/install.sh --help
```

**Features:**
- Automatic OS detection
- Dependency checking
- Service user/group creation
- Directory setup with proper permissions
- Systemd service installation
- Init.d script installation
- Firewall configuration
- Log rotation setup
- Installation verification

### 2. Manual Installation

#### Prerequisites

```bash
# Create service user and group
sudo groupadd -r ntp
sudo useradd -r -g ntp -s /bin/false -d /var/lib/simple-ntpd ntp

# Create required directories
sudo mkdir -p /etc/simple-ntpd
sudo mkdir -p /var/log/simple-ntpd
sudo mkdir -p /var/lib/simple-ntpd
sudo mkdir -p /var/run/simple-ntpd

# Set permissions
sudo chown -R ntp:ntp /var/log/simple-ntpd /var/lib/simple-ntpd /var/run/simple-ntpd
sudo chmod 755 /var/log/simple-ntpd /var/lib/simple-ntpd /var/run/simple-ntpd
sudo chmod 755 /etc/simple-ntpd
```

#### Build and Install

```bash
# Build the project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Install
sudo make install

# Copy configuration
sudo cp ../config/simple-ntpd.conf.example /etc/simple-ntpd/simple-ntpd.conf
sudo chown ntp:ntp /etc/simple-ntpd/simple-ntpd.conf
sudo chmod 644 /etc/simple-ntpd/simple-ntpd.conf
```

## Service Management

### Systemd (Modern Linux Systems)

#### Installation

```bash
# Copy service files
sudo cp deployment/systemd/*.service /etc/systemd/system/
sudo cp deployment/systemd/*.socket /etc/systemd/system/
sudo cp deployment/systemd/*.target /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable services
sudo systemctl enable simple-ntpd.socket
sudo systemctl enable simple-ntpd.service
```

#### Management

```bash
# Start the service
sudo systemctl start simple-ntpd

# Check status
sudo systemctl status simple-ntpd

# Stop the service
sudo systemctl stop simple-ntpd

# Restart the service
sudo systemctl restart simple-ntpd

# Reload configuration
sudo systemctl reload simple-ntpd

# View logs
sudo journalctl -u simple-ntpd -f
```

### Init.d (Traditional Linux Systems)

#### Installation

```bash
# Copy init.d script
sudo cp deployment/init.d/simple-ntpd /etc/init.d/
sudo chmod 755 /etc/init.d/simple-ntpd

# Enable service
sudo update-rc.d simple-ntpd defaults  # Debian/Ubuntu
# OR
sudo chkconfig --add simple-ntpd       # RedHat/CentOS
sudo chkconfig --level 2345 simple-ntpd on
```

#### Management

```bash
# Start the service
sudo /etc/init.d/simple-ntpd start

# Check status
sudo /etc/init.d/simple-ntpd status

# Stop the service
sudo /etc/init.d/simple-ntpd stop

# Restart the service
sudo /etc/init.d/simple-ntpd restart

# Reload configuration
sudo /etc/init.d/simple-ntpd reload

# Test configuration
sudo /etc/init.d/simple-ntpd configtest
```

## Configuration

### Configuration File

The main configuration file is located at `/etc/simple-ntpd/simple-ntpd.conf`. An example configuration is provided in `config/simple-ntpd.conf.example`.

Key configuration sections:
- **Network**: Listen address, port, IPv6 support
- **NTP Server**: Stratum level, reference clock, upstream servers
- **Logging**: Log levels, output destinations
- **Security**: Authentication, access control
- **Performance**: Worker threads, packet sizes

### Environment Variables

The following environment variables can be used to override configuration:

```bash
export SIMPLE_NTPD_LISTEN_ADDRESS="0.0.0.0"
export SIMPLE_NTPD_LISTEN_PORT="123"
export SIMPLE_NTPD_STRATUM="2"
export SIMPLE_NTPD_LOG_LEVEL="INFO"
```

## Security Considerations

### Service User

The daemon runs as the `ntp` user with minimal privileges:
- No shell access (`/bin/false`)
- Restricted home directory (`/var/lib/simple-ntpd`)
- Minimal file permissions

### File Permissions

```bash
# Configuration files
/etc/simple-ntpd/simple-ntpd.conf     # 644, ntp:ntp

# Runtime directories
/var/run/simple-ntpd/                 # 755, ntp:ntp
/var/log/simple-ntpd/                 # 755, ntp:ntp
/var/lib/simple-ntpd/                 # 755, ntp:ntp

# Binary and libraries
/usr/local/bin/simple-ntpd            # 755, root:root
/usr/local/lib/libsimple-ntpd-core.a # 644, root:root
```

### Network Security

- Only UDP port 123 is opened
- Firewall rules are automatically configured
- No incoming connections are accepted (UDP only)

## Monitoring and Logging

### Log Files

- **Application logs**: `/var/log/simple-ntpd/`
- **System logs**: `journalctl -u simple-ntpd` (systemd)
- **Log rotation**: Configured via `/etc/logrotate.d/simple-ntpd`

### Health Checks

```bash
# Check service status
sudo systemctl is-active simple-ntpd

# Check NTP service
ntpdate -q localhost

# Check listening ports
sudo netstat -ulnp | grep :123
sudo ss -ulnp | grep :123

# Check process
ps aux | grep simple-ntpd
```

## Troubleshooting

### Common Issues

1. **Permission Denied**
   ```bash
   # Check user/group
   id ntp
   # Check file permissions
   ls -la /etc/simple-ntpd/
   ```

2. **Port Already in Use**
   ```bash
   # Check what's using port 123
   sudo lsof -i :123
   sudo netstat -ulnp | grep :123
   ```

3. **Service Won't Start**
   ```bash
   # Check logs
   sudo journalctl -u simple-ntpd -n 50
   # Check configuration
   sudo /etc/init.d/simple-ntpd configtest
   ```

4. **Firewall Issues**
   ```bash
   # Check firewall status
   sudo firewall-cmd --list-services
   sudo ufw status
   ```

### Debug Mode

```bash
# Run in foreground with debug logging
sudo simple-ntpd --verbose --foreground start

# Check configuration
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf
```

## Platform-Specific Notes

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt update
sudo apt install build-essential cmake libc6-dev

# Service management
sudo systemctl enable simple-ntpd
sudo systemctl start simple-ntpd
```

### CentOS/RHEL

```bash
# Install dependencies
sudo yum groupinstall "Development Tools"
sudo yum install cmake glibc-devel

# Service management
sudo systemctl enable simple-ntpd
sudo systemctl start simple-ntpd
```

### macOS

```bash
# Install dependencies
brew install cmake

# Build with specific architecture
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..
make
```

### Windows

```bash
# Install Visual Studio Build Tools
# Use CMake GUI or command line
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## Uninstallation

### Complete Removal

```bash
# Stop and disable services
sudo systemctl stop simple-ntpd
sudo systemctl disable simple-ntpd

# Remove service files
sudo rm -f /etc/systemd/system/simple-ntpd.*

# Remove init.d script
sudo rm -f /etc/init.d/simple-ntpd

# Remove application files
sudo rm -f /usr/local/bin/simple-ntpd
sudo rm -f /usr/local/lib/libsimple-ntpd-core.a
sudo rm -rf /usr/local/lib/cmake/simple-ntpd

# Remove configuration and data
sudo rm -rf /etc/simple-ntpd
sudo rm -rf /var/log/simple-ntpd
sudo rm -rf /var/lib/simple-ntpd
sudo rm -rf /var/run/simple-ntpd

# Remove service user
sudo userdel ntp
sudo groupdel ntp

# Remove log rotation
sudo rm -f /etc/logrotate.d/simple-ntpd
```

## Support

For deployment issues and questions:

- Check the troubleshooting section above
- Review system logs and service status
- Ensure all prerequisites are met
- Verify file permissions and ownership
- Check firewall and network configuration

For additional help, refer to the main project README or create an issue in the project repository.
