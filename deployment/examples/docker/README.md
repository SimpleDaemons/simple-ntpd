# Docker Deployment Guide

This guide covers deploying simple-ntpd using Docker containers.

## Quick Start

### 1. Clone the Repository

```bash
git clone <repository-url>
cd simple-ntpd
```

### 2. Create Configuration Directory

```bash
mkdir -p deployment/examples/docker/config
mkdir -p deployment/examples/docker/logs
cp config/examples/simple/simple-ntpd.conf.example deployment/examples/docker/config/simple-ntpd.conf
```

### 3. Build and Run

```bash
cd deployment/examples/docker
docker-compose up -d
```

### 4. Verify Deployment

```bash
# Check container status
docker-compose ps

# Check logs
docker-compose logs -f simple-ntpd

# Test NTP service
ntpdate -q localhost
```

## Configuration

### Environment Variables

The following environment variables can be set in the docker-compose.yml:

- `SIMPLE_NTPD_LISTEN_ADDRESS`: Bind address (default: 0.0.0.0)
- `SIMPLE_NTPD_LISTEN_PORT`: Bind port (default: 123)
- `SIMPLE_NTPD_STRATUM`: Stratum level (default: 2)
- `SIMPLE_NTPD_LOG_LEVEL`: Log level (default: INFO)

### Volume Mounts

- `./config:/etc/simple-ntpd:ro`: Configuration files (read-only)
- `./logs:/var/log/simple-ntpd`: Log files

## Production Deployment

### 1. Create Production Configuration

```bash
# Copy production config
cp config/examples/production/enterprise.conf deployment/examples/docker/config/simple-ntpd.conf

# Edit configuration
nano deployment/examples/docker/config/simple-ntpd.conf
```

### 2. Set Resource Limits

```yaml
services:
  simple-ntpd:
    # ... existing configuration ...
    deploy:
      resources:
        limits:
          memory: 256M
          cpus: '0.5'
        reservations:
          memory: 128M
          cpus: '0.25'
```

### 3. Enable Logging Driver

```yaml
services:
  simple-ntpd:
    # ... existing configuration ...
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
```

## Monitoring

### Health Checks

The container includes a health check that verifies NTP service availability:

```bash
# Check health status
docker inspect --format='{{.State.Health.Status}}' simple-ntpd
```

### Log Monitoring

```bash
# Follow logs in real-time
docker-compose logs -f simple-ntpd

# Search logs for errors
docker-compose logs simple-ntpd | grep ERROR

# Export logs
docker-compose logs simple-ntpd > ntp-logs.txt
```

## Troubleshooting

### Common Issues

1. **Port Already in Use**
   ```bash
   # Check what's using port 123
   sudo lsof -i :123

   # Stop conflicting service
   sudo systemctl stop ntp
   ```

2. **Permission Issues**
   ```bash
   # Fix log directory permissions
   sudo chown -R 1000:1000 deployment/examples/docker/logs
   ```

3. **Configuration Errors**
   ```bash
   # Validate configuration
   docker-compose exec simple-ntpd simple-ntpd --test-config
   ```

### Debug Mode

```bash
# Run with debug logging
docker-compose run --rm simple-ntpd simple-ntpd --verbose --foreground
```

## Scaling

### Multiple Instances

```yaml
services:
  simple-ntpd:
    # ... existing configuration ...
    deploy:
      replicas: 3
      endpoint_mode: dnsrr
```

### Load Balancing

Use an external load balancer (HAProxy, nginx) to distribute NTP requests across multiple containers.

## Security Considerations

- The container runs as a non-root user
- Configuration files are mounted as read-only
- Only UDP port 123 is exposed
- Network isolation using custom bridge network

## Backup and Recovery

### Backup Configuration

```bash
# Backup configuration
docker cp simple-ntpd:/etc/simple-ntpd/simple-ntpd.conf ./backup/
```

### Restore Configuration

```bash
# Restore configuration
docker cp ./backup/simple-ntpd.conf simple-ntpd:/etc/simple-ntpd/
docker-compose restart simple-ntpd
```

## Performance Tuning

### Container Optimization

```yaml
services:
  simple-ntpd:
    # ... existing configuration ...
    ulimits:
      nofile:
        soft: 65536
        hard: 65536
    sysctls:
      - net.core.rmem_max=26214400
      - net.core.wmem_max=26214400
```

### Host System Tuning

```bash
# Increase UDP buffer sizes
echo 'net.core.rmem_max=26214400' >> /etc/sysctl.conf
echo 'net.core.wmem_max=26214400' >> /etc/sysctl.conf
sysctl -p
```
