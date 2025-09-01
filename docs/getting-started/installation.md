# Installation Guide

This guide covers installing Simple NTP Daemon on all supported platforms.

## Prerequisites

### System Requirements
- **Operating System**: Linux, macOS, Windows, or containerized environments
- **Architecture**: x86_64, ARM64, or ARM32
- **Memory**: Minimum 128MB RAM (512MB recommended for production)
- **Storage**: Minimum 100MB free space
- **Network**: UDP port 123 available

### Build Dependencies
- **Linux**: gcc, cmake, make, libc6-dev
- **macOS**: Xcode Command Line Tools, cmake
- **Windows**: Visual Studio Build Tools, cmake
- **Container**: Base image with build tools

## Installation Methods

### 1. Automated Installation (Recommended)

The automated installer handles all dependencies and configuration:

```bash
# Clone repository
git clone https://github.com/your-repo/simple-ntpd.git
cd simple-ntpd

# Run automated installer
sudo ./scripts/install.sh

# Or with options
sudo ./scripts/install.sh --production --skip-build
```

**Installation Options:**
- `--production`: Production-ready configuration
- `--skip-build`: Use pre-built binary
- `--skip-systemd`: Skip systemd service installation
- `--help`: Show all options

### 2. Manual Installation

#### Step 1: Build from Source

```bash
# Clone repository
git clone https://github.com/your-repo/simple-ntpd.git
cd simple-ntpd

# Create build directory
mkdir build && cd build

# Configure build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)

# Install
sudo make install
```

#### Step 2: Create Service User

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
```

#### Step 3: Configure Service

```bash
# Copy configuration
sudo cp ../config/examples/simple/simple-ntpd.conf.example /etc/simple-ntpd/simple-ntpd.conf

# Set permissions
sudo chown ntp:ntp /etc/simple-ntpd/simple-ntpd.conf
sudo chmod 644 /etc/simple-ntpd/simple-ntpd.conf
```

### 3. Platform-Specific Installation

#### Linux (systemd)

```bash
# Copy service files
sudo cp ../deployment/systemd/*.service /etc/systemd/system/
sudo cp ../deployment/systemd/*.socket /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable and start services
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
sudo chkconfig --level 2345 simple-ntpd on
```

#### macOS

```bash
# Install dependencies
brew install cmake

# Build with architecture support
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..

# Install launchd service
sudo cp ../deployment/launchd/com.simpledaemons.simple-ntpd.plist /Library/LaunchDaemons/
sudo chown root:wheel /Library/LaunchDaemons/com.simpledaemons.simple-ntpd.plist

# Load and start service
sudo launchctl load /Library/LaunchDaemons/com.simpledaemons.simple-ntpd.plist
sudo launchctl start com.simpledaemons.simple-ntpd
```

#### Windows

```bash
# Build with Visual Studio
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release

# Install service (run as Administrator)
cd ../deployment/windows
.\install-service.bat

# Or use PowerShell
.\install-service.ps1
```

### 4. Container Installation

#### Docker

```bash
# Build image
docker build -t simple-ntpd .

# Run container
docker run -d \
  --name simple-ntpd \
  -p 123:123/udp \
  -v $(pwd)/config:/etc/simple-ntpd:ro \
  simple-ntpd
```

#### Docker Compose

```bash
cd deployment/examples/docker
docker-compose up -d
```

#### Kubernetes

```bash
cd deployment/examples/kubernetes
kubectl apply -f .
```

## Package Manager Installation

### Ubuntu/Debian

```bash
# Add repository (if available)
sudo apt update
sudo apt install simple-ntpd

# Or install from package file
sudo dpkg -i simple-ntpd_*.deb
sudo apt-get install -f  # Fix dependencies if needed
```

### CentOS/RHEL

```bash
# Install from package
sudo yum install simple-ntpd
# OR
sudo dnf install simple-ntpd

# Or install from RPM file
sudo rpm -i simple-ntpd-*.rpm
```

### macOS

```bash
# Install via Homebrew (if available)
brew install simple-ntpd

# Or install from package
sudo installer -pkg simple-ntpd-*.pkg -target /
```

## Post-Installation Steps

### 1. Verify Installation

```bash
# Check service status
sudo systemctl status simple-ntpd  # Linux systemd
# OR
sudo launchctl list | grep simple-ntpd  # macOS
# OR
sc query simple-ntpd  # Windows

# Test NTP service
ntpdate -q localhost

# Check listening ports
sudo netstat -ulnp | grep :123
sudo ss -ulnp | grep :123
```

### 2. Configure Firewall

```bash
# UFW (Ubuntu)
sudo ufw allow 123/udp

# firewalld (RHEL/CentOS)
sudo firewall-cmd --permanent --add-service=ntp
sudo firewall-cmd --reload

# iptables
sudo iptables -A INPUT -p udp --dport 123 -j ACCEPT
```

### 3. Test Configuration

```bash
# Test configuration file
sudo simple-ntpd --test-config

# Run in foreground for testing
sudo simple-ntpd --verbose --foreground
```

## Configuration

### Basic Configuration

```ini
[network]
listen_address = 0.0.0.0
listen_port = 123

[ntp_server]
stratum = 2
upstream_servers = ["pool.ntp.org", "time.nist.gov"]

[logging]
level = "INFO"
```

### Environment Variables

```bash
export SIMPLE_NTPD_LISTEN_ADDRESS="0.0.0.0"
export SIMPLE_NTPD_LISTEN_PORT="123"
export SIMPLE_NTPD_STRATUM="2"
export SIMPLE_NTPD_LOG_LEVEL="INFO"
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

### Getting Help

- Check the [Troubleshooting Guide](../troubleshooting/README.md)
- Review system logs and service status
- Verify all prerequisites are met
- Check file permissions and ownership

## Next Steps

After successful installation:

1. **Configure Monitoring**: Set up health checks and metrics
2. **Security Hardening**: Apply security best practices
3. **Performance Tuning**: Optimize for your workload
4. **Backup Strategy**: Implement configuration backup
5. **Documentation**: Document your specific deployment

## Support

For installation issues:

- Check the troubleshooting section above
- Review system logs and service status
- Ensure all prerequisites are met
- Verify file permissions and ownership
- Check firewall and network configuration

For additional help, refer to the main project README or create an issue in the project repository.
