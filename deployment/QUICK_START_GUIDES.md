# Quick Start Guides

This document provides quick-start instructions for different deployment methods of simple-ntpd.

## Linux Quick Start (systemd)

### Prerequisites
- Linux distribution with systemd
- Root or sudo access
- Build tools (gcc, cmake, make)

### 1. Build and Install
```bash
# Clone repository
git clone <repository-url>
cd simple-ntpd

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Install
sudo make install
```

### 2. Configure Service
```bash
# Create service user
sudo groupadd -r ntp
sudo useradd -r -g ntp -s /bin/false -d /var/lib/simple-ntpd ntp

# Create directories
sudo mkdir -p /etc/simple-ntpd /var/log/simple-ntpd /var/lib/simple-ntpd

# Copy configuration
sudo cp ../config/examples/simple/simple-ntpd.conf.example /etc/simple-ntpd/simple-ntpd.conf

# Set permissions
sudo chown -R ntp:ntp /var/log/simple-ntpd /var/lib/simple-ntpd
sudo chmod 644 /etc/simple-ntpd/simple-ntpd.conf
```

### 3. Install Service
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

### 4. Verify Installation
```bash
# Check status
sudo systemctl status simple-ntpd

# Test NTP service
ntpdate -q localhost

# Check logs
sudo journalctl -u simple-ntpd -f
```

## Docker Quick Start

### Prerequisites
- Docker and Docker Compose installed
- Git

### 1. Clone and Setup
```bash
git clone <repository-url>
cd simple-ntpd

# Create directories
mkdir -p deployment/examples/docker/config
mkdir -p deployment/examples/docker/logs

# Copy configuration
cp config/examples/simple/simple-ntpd.conf.example deployment/examples/docker/config/simple-ntpd.conf
```

### 2. Deploy
```bash
cd deployment/examples/docker
docker-compose up -d
```

### 3. Verify
```bash
# Check status
docker-compose ps

# Test service
ntpdate -q localhost

# View logs
docker-compose logs -f simple-ntpd
```

## Kubernetes Quick Start

### Prerequisites
- Kubernetes cluster (v1.19+)
- kubectl configured
- Git

### 1. Clone Repository
```bash
git clone <repository-url>
cd simple-ntpd
```

### 2. Deploy
```bash
cd deployment/examples/kubernetes

# Create namespace and deploy
kubectl apply -f namespace.yaml
kubectl apply -f serviceaccount.yaml
kubectl apply -f configmap.yaml
kubectl apply -f deployment.yaml
kubectl apply -f service.yaml
```

### 3. Verify
```bash
# Check resources
kubectl get all -n ntp

# Check logs
kubectl logs -f deployment/simple-ntpd -n ntp

# Test service
kubectl run test-pod --image=busybox --rm -it --restart=Never -- nslookup simple-ntpd-service.ntp
```

## macOS Quick Start

### Prerequisites
- macOS 10.14+
- Homebrew
- Git

### 1. Install Dependencies
```bash
brew install cmake
```

### 2. Build and Install
```bash
git clone <repository-url>
cd simple-ntpd

mkdir build && cd build
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..
make
sudo make install
```

### 3. Configure Service
```bash
# Create directories
sudo mkdir -p /etc/simple-ntpd /var/log/simple-ntpd

# Copy configuration
sudo cp ../config/examples/simple/simple-ntpd.conf.example /etc/simple-ntpd/simple-ntpd.conf

# Install launchd service
sudo cp ../deployment/launchd/com.simpledaemons.simple-ntpd.plist /Library/LaunchDaemons/
sudo chown root:wheel /Library/LaunchDaemons/com.simpledaemons.simple-ntpd.plist
```

### 4. Start Service
```bash
sudo launchctl load /Library/LaunchDaemons/com.simpledaemons.simple-ntpd.plist
sudo launchctl start com.simpledaemons.simple-ntpd
```

### 5. Verify
```bash
# Check status
sudo launchctl list | grep simple-ntpd

# Test service
ntpdate -q localhost

# Check logs
tail -f /var/log/simple-ntpd/simple-ntpd.log
```

## Windows Quick Start

### Prerequisites
- Windows 10/11 or Windows Server 2016+
- Visual Studio Build Tools or Visual Studio
- Git
- Administrator access

### 1. Build
```bash
git clone <repository-url>
cd simple-ntpd

mkdir build && cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release
```

### 2. Install Service
```bash
# Run as Administrator
cd deployment/windows
.\install-service.bat

# Or use PowerShell
.\install-service.ps1
```

### 3. Configure
```bash
# Copy configuration
copy ..\..\config\examples\simple\simple-ntpd.conf.example C:\Program Files\simple-ntpd\simple-ntpd.conf

# Edit configuration as needed
notepad C:\Program Files\simple-ntpd\simple-ntpd.conf
```

### 4. Start Service
```bash
# Start service
net start simple-ntpd

# Check status
sc query simple-ntpd
```

### 5. Verify
```bash
# Test service
w32tm /stripchart /computer:localhost /dataonly /samples:5

# Check Windows Event Log
eventvwr.msc
```

## Automated Installation

### Using Install Script
```bash
git clone <repository-url>
cd simple-ntpd

# Run automated installer
sudo ./scripts/install.sh

# Or with options
sudo ./scripts/install.sh --skip-build --production
```

### Using Package Manager
```bash
# Ubuntu/Debian (if package available)
sudo apt update
sudo apt install simple-ntpd

# CentOS/RHEL (if package available)
sudo yum install simple-ntpd
# or
sudo dnf install simple-ntpd
```

## Configuration Quick Start

### Basic Configuration
```bash
# Copy example configuration
sudo cp config/examples/simple/simple-ntpd.conf.example /etc/simple-ntpd/simple-ntpd.conf

# Edit configuration
sudo nano /etc/simple-ntpd/simple-ntpd.conf
```

### Key Settings to Modify
```ini
[network]
listen_address = 0.0.0.0  # Bind to all interfaces
listen_port = 123          # Standard NTP port

[ntp_server]
stratum = 2                # Stratum level
upstream_servers = [       # Upstream NTP servers
    "pool.ntp.org",
    "time.nist.gov"
]

[logging]
level = "INFO"             # Log level
```

### Environment Variables
```bash
export SIMPLE_NTPD_LISTEN_ADDRESS="0.0.0.0"
export SIMPLE_NTPD_STRATUM="2"
export SIMPLE_NTPD_LOG_LEVEL="INFO"
```

## Troubleshooting Quick Start

### Common Issues

1. **Service Won't Start**
   ```bash
   # Check logs
   sudo journalctl -u simple-ntpd -n 50

   # Check configuration
   sudo simple-ntpd --test-config
   ```

2. **Port Already in Use**
   ```bash
   # Check what's using port 123
   sudo lsof -i :123

   # Stop conflicting service
   sudo systemctl stop ntp
   ```

3. **Permission Issues**
   ```bash
   # Fix permissions
   sudo chown -R ntp:ntp /var/log/simple-ntpd
   sudo chmod 755 /var/log/simple-ntpd
   ```

### Debug Mode
```bash
# Run in foreground with debug
sudo simple-ntpd --verbose --foreground

# Test configuration
sudo simple-ntpd --test-config --config /etc/simple-ntpd/simple-ntpd.conf
```

## Next Steps

After successful deployment:

1. **Configure Monitoring**: Set up health checks and metrics
2. **Security Hardening**: Review and apply security best practices
3. **Performance Tuning**: Optimize for your workload
4. **Backup Strategy**: Implement configuration and data backup
5. **Documentation**: Document your specific deployment

## Support

- Check the main deployment README for detailed information
- Review troubleshooting sections
- Check system logs for error messages
- Verify network connectivity and firewall rules
