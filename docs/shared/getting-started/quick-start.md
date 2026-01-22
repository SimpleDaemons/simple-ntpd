# Quick Start Guide

Get your Simple NTP Daemon server running in 5 minutes! This guide covers the fastest path to a working NTP server.

## 🚀 Prerequisites

- **Operating System**: Linux, macOS, or Windows
- **Build Tools**: CMake 3.15+, C++17 compiler
- **Network**: UDP port 123 accessible (may require admin/root privileges)

## ⚡ 5-Minute Setup

### 1. Clone and Build (2 minutes)

```bash
# Clone the repository
git clone https://github.com/SimpleDaemons/simple-ntpd.git
cd simple-ntpd

# Build the project
make build
```

### 2. Create Configuration (1 minute)

```bash
# Copy the example configuration
cp config/examples/simple/simple-ntpd.conf.example config/simple-ntpd.conf

# Edit the configuration (optional)
nano config/simple-ntpd.conf
```

### 3. Start the Server (1 minute)

```bash
# Run the server
sudo ./build/bin/simple-ntpd -c config/simple-ntpd.conf
```

### 4. Test the Server (1 minute)

```bash
# In another terminal, test with ntpdate or chrony
ntpdate -q localhost

# Or test with a simple NTP client
echo "test" | nc -u localhost 123
```

## ✅ Success Indicators

Your NTP server is working if you see:
- Server startup messages in the console
- No error messages
- NTP client can query the server
- Server responds to time requests

## 🔧 Basic Configuration

The default configuration provides:
- **Listen Address**: 0.0.0.0 (all interfaces)
- **Port**: 123 (standard NTP port)
- **Log Level**: INFO
- **Console Logging**: Enabled
- **IPv6 Support**: Enabled

## 🚨 Common Issues

### Permission Denied
```bash
# NTP port 123 requires root privileges
sudo ./build/bin/simple-ntpd -c config/simple-ntpd.conf
```

### Port Already in Use
```bash
# Check what's using port 123
sudo netstat -tulpn | grep :123

# Stop conflicting service (e.g., systemd-timesyncd)
sudo systemctl stop systemd-timesyncd
```

### Build Failures
```bash
# Clean and rebuild
make clean
make build
```

## 📱 Next Steps

Now that you have a basic NTP server running:

1. **Configure Clients**: Point your network devices to this server
2. **Customize Settings**: Modify configuration for your environment
3. **Production Setup**: See [Production Deployment](../deployment/production.md)
4. **Security**: Review [Security Hardening](../deployment/security.md)

## 🔗 Quick Reference

| Command | Description |
|---------|-------------|
| `make build` | Build the project |
| `make clean` | Clean build artifacts |
| `make test` | Run test suite |
| `make install` | Install system-wide |

## 📞 Need Help?

- **Documentation**: [Full Documentation](../README.md)
- **Issues**: [GitHub Issues](https://github.com/blburns/simple-ntpd/issues)
- **Discussions**: [GitHub Discussions](https://github.com/blburns/simple-ntpd/discussions)

---

*Congratulations! You now have a working NTP server. Continue to the [First Steps](first-steps.md) guide for more detailed configuration.*
