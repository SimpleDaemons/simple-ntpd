# simple-ntpd Deployment

This directory contains deployment configurations and examples for simple-ntpd.

## Directory Structure

```
deployment/
├── systemd/                    # Linux systemd service files
│   └── simple-ntpd.service
├── launchd/                    # macOS launchd service files
│   └── com.simple-ntpd.simple-ntpd.plist
├── logrotate.d/                # Linux log rotation configuration
│   └── simple-ntpd
├── windows/                    # Windows service management
│   └── simple-ntpd.service.bat
└── examples/                   # Deployment examples
    └── docker/                 # Docker deployment examples
        ├── docker-compose.yml
        └── README.md
```

## Platform-Specific Deployment

### Linux (systemd)

1. **Install the service file:**
   ```bash
   sudo cp deployment/systemd/simple-ntpd.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```

2. **Create user and group:**
   ```bash
   sudo useradd --system --no-create-home --shell /bin/false simple-ntpd
   ```

3. **Enable and start the service:**
   ```bash
   sudo systemctl enable simple-ntpd
   sudo systemctl start simple-ntpd
   ```

4. **Check status:**
   ```bash
   sudo systemctl status simple-ntpd
   sudo journalctl -u simple-ntpd -f
   ```

### macOS (launchd)

1. **Install the plist file:**
   ```bash
   sudo cp deployment/launchd/com.simple-ntpd.simple-ntpd.plist /Library/LaunchDaemons/
   sudo chown root:wheel /Library/LaunchDaemons/com.simple-ntpd.simple-ntpd.plist
   ```

2. **Load and start the service:**
   ```bash
   sudo launchctl load /Library/LaunchDaemons/com.simple-ntpd.simple-ntpd.plist
   sudo launchctl start com.simple-ntpd.simple-ntpd
   ```

3. **Check status:**
   ```bash
   sudo launchctl list | grep simple-ntpd
   tail -f /var/log/simple-ntpd.log
   ```

### Windows

1. **Run as Administrator:**
   ```cmd
   # Install service
   deployment\windows\simple-ntpd.service.bat install
   
   # Start service
   deployment\windows\simple-ntpd.service.bat start
   
   # Check status
   deployment\windows\simple-ntpd.service.bat status
   ```

2. **Service management:**
   ```cmd
   # Stop service
   deployment\windows\simple-ntpd.service.bat stop
   
   # Restart service
   deployment\windows\simple-ntpd.service.bat restart
   
   # Uninstall service
   deployment\windows\simple-ntpd.service.bat uninstall
   ```

## Log Rotation (Linux)

1. **Install logrotate configuration:**
   ```bash
   sudo cp deployment/logrotate.d/simple-ntpd /etc/logrotate.d/
   ```

2. **Test logrotate configuration:**
   ```bash
   sudo logrotate -d /etc/logrotate.d/simple-ntpd
   ```

3. **Force log rotation:**
   ```bash
   sudo logrotate -f /etc/logrotate.d/simple-ntpd
   ```

## Docker Deployment

See [examples/docker/README.md](examples/docker/README.md) for detailed Docker deployment instructions.

### Quick Start

```bash
# Build and run with Docker Compose
cd deployment/examples/docker
docker-compose up -d

# Check status
docker-compose ps
docker-compose logs simple-ntpd
```

## Configuration

### Service Configuration

Each platform has specific configuration requirements:

- **Linux**: Edit `/etc/systemd/system/simple-ntpd.service`
- **macOS**: Edit `/Library/LaunchDaemons/com.simple-ntpd.simple-ntpd.plist`
- **Windows**: Edit the service binary path in the batch file

### Application Configuration

Place your application configuration in:
- **Linux/macOS**: `/etc/simple-ntpd/simple-ntpd.conf`
- **Windows**: `%PROGRAMFILES%\simple-ntpd\simple-ntpd.conf`

## Security Considerations

### User and Permissions

1. **Create dedicated user:**
   ```bash
   # Linux
   sudo useradd --system --no-create-home --shell /bin/false simple-ntpd
   
   # macOS
   sudo dscl . -create /Users/_simple-ntpd UserShell /usr/bin/false
   sudo dscl . -create /Users/_simple-ntpd UniqueID 200
   sudo dscl . -create /Users/_simple-ntpd PrimaryGroupID 200
   sudo dscl . -create /Groups/_simple-ntpd GroupID 200
   ```

2. **Set proper permissions:**
   ```bash
   # Configuration files
   sudo chown root:simple-ntpd /etc/simple-ntpd/simple-ntpd.conf
   sudo chmod 640 /etc/simple-ntpd/simple-ntpd.conf
   
   # Log files
   sudo chown simple-ntpd:simple-ntpd /var/log/simple-ntpd/
   sudo chmod 755 /var/log/simple-ntpd/
   ```

### Firewall Configuration

Configure firewall rules as needed:

```bash
# Linux (ufw)
sudo ufw allow 123/tcp

# Linux (firewalld)
sudo firewall-cmd --permanent --add-port=123/tcp
sudo firewall-cmd --reload

# macOS
sudo pfctl -f /etc/pf.conf
```

## Monitoring

### Health Checks

1. **Service status:**
   ```bash
   # Linux
   sudo systemctl is-active simple-ntpd
   
   # macOS
   sudo launchctl list | grep simple-ntpd
   
   # Windows
   sc query simple-ntpd
   ```

2. **Port availability:**
   ```bash
   netstat -tlnp | grep 123
   ss -tlnp | grep 123
   ```

3. **Process monitoring:**
   ```bash
   ps aux | grep simple-ntpd
   top -p $(pgrep simple-ntpd)
   ```

### Log Monitoring

1. **Real-time logs:**
   ```bash
   # Linux
   sudo journalctl -u simple-ntpd -f
   
   # macOS
   tail -f /var/log/simple-ntpd.log
   
   # Windows
   # Use Event Viewer or PowerShell Get-WinEvent
   ```

2. **Log analysis:**
   ```bash
   # Search for errors
   sudo journalctl -u simple-ntpd --since "1 hour ago" | grep -i error
   
   # Count log entries
   sudo journalctl -u simple-ntpd --since "1 day ago" | wc -l
   ```

## Troubleshooting

### Common Issues

1. **Service won't start:**
   - Check configuration file syntax
   - Verify user permissions
   - Check port availability
   - Review service logs

2. **Permission denied:**
   - Ensure service user exists
   - Check file permissions
   - Verify directory ownership

3. **Port already in use:**
   - Check what's using the port: `netstat -tlnp | grep 123`
   - Stop conflicting service or change port

4. **Service stops unexpectedly:**
   - Check application logs
   - Verify resource limits
   - Review system logs

### Debug Mode

Run the service in debug mode for troubleshooting:

```bash
# Linux/macOS
sudo -u simple-ntpd /usr/local/bin/simple-ntpd --debug

# Windows
simple-ntpd.exe --debug
```

### Log Levels

Adjust log level for more verbose output:

```bash
# Set log level in configuration
log_level = debug

# Or via environment variable
export SIMPLE-NTPD_LOG_LEVEL=debug
```

## Backup and Recovery

### Configuration Backup

```bash
# Backup configuration
sudo tar -czf simple-ntpd-config-backup-$(date +%Y%m%d).tar.gz /etc/simple-ntpd/

# Backup logs
sudo tar -czf simple-ntpd-logs-backup-$(date +%Y%m%d).tar.gz /var/log/simple-ntpd/
```

### Service Recovery

```bash
# Stop service
sudo systemctl stop simple-ntpd

# Restore configuration
sudo tar -xzf simple-ntpd-config-backup-YYYYMMDD.tar.gz -C /

# Start service
sudo systemctl start simple-ntpd
```

## Updates

### Service Update Process

1. **Stop service:**
   ```bash
   sudo systemctl stop simple-ntpd
   ```

2. **Backup current version:**
   ```bash
   sudo cp /usr/local/bin/simple-ntpd /usr/local/bin/simple-ntpd.backup
   ```

3. **Install new version:**
   ```bash
   sudo cp simple-ntpd /usr/local/bin/
   sudo chmod +x /usr/local/bin/simple-ntpd
   ```

4. **Start service:**
   ```bash
   sudo systemctl start simple-ntpd
   ```

5. **Verify update:**
   ```bash
   sudo systemctl status simple-ntpd
   simple-ntpd --version
   ```
