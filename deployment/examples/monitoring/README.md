# Monitoring and Observability Guide

This guide covers setting up comprehensive monitoring and observability for simple-ntpd deployments.

## Overview

Monitoring simple-ntpd involves tracking:
- **Service Health**: Availability and responsiveness
- **Performance Metrics**: Response times, throughput, resource usage
- **Operational Metrics**: Client connections, sync status, errors
- **System Metrics**: CPU, memory, disk, network usage

## Monitoring Stack Components

### 1. Metrics Collection
- **Prometheus**: Time-series database for metrics storage
- **Node Exporter**: System-level metrics collection
- **Custom Metrics**: Application-specific NTP metrics

### 2. Visualization
- **Grafana**: Dashboards and visualization
- **Prometheus UI**: Basic query interface

### 3. Alerting
- **Alertmanager**: Alert routing and notification
- **Notification Channels**: Email, Slack, PagerDuty, etc.

### 4. Logging
- **Log Aggregation**: Centralized log collection
- **Log Analysis**: Search and analysis tools

## Quick Start

### 1. Install Prometheus

```bash
# Download Prometheus
wget https://github.com/prometheus/prometheus/releases/download/v2.45.0/prometheus-2.45.0.linux-amd64.tar.gz
tar xvf prometheus-*.tar.gz
cd prometheus-*

# Create configuration
cat > prometheus.yml << EOF
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'simple-ntpd'
    static_configs:
      - targets: ['localhost:8080']
EOF

# Start Prometheus
./prometheus --config.file=prometheus.yml
```

### 2. Install Grafana

```bash
# Ubuntu/Debian
sudo apt-get install -y software-properties-common
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
sudo apt-get update
sudo apt-get install grafana

# Start Grafana
sudo systemctl start grafana-server
sudo systemctl enable grafana-server
```

### 3. Configure Data Source

1. Open Grafana (http://localhost:3000)
2. Default credentials: admin/admin
3. Add Prometheus data source (http://localhost:9090)

## Metrics Endpoints

### Health Check Endpoint

```bash
# Check service health
curl http://localhost:8080/health

# Response format
{
  "status": "healthy",
  "timestamp": "2024-01-01T00:00:00Z",
  "uptime": 3600,
  "version": "1.0.0"
}
```

### Metrics Endpoint

```bash
# Get Prometheus metrics
curl http://localhost:8080/metrics

# Key metrics include:
# - ntp_up: Service availability (1 = up, 0 = down)
# - ntp_response_time_seconds: Response time histogram
# - ntp_requests_total: Total requests counter
# - ntp_errors_total: Total errors counter
# - ntp_active_connections: Current active connections
# - ntp_sync_status: Synchronization status
# - ntp_stratum: Current stratum level
```

## Grafana Dashboards

### 1. Import Dashboard

```bash
# Download dashboard JSON
wget https://raw.githubusercontent.com/your-repo/simple-ntpd/main/deployment/examples/monitoring/grafana-dashboard.json

# Import in Grafana:
# 1. Click + icon → Import
# 2. Upload JSON file
# 3. Select Prometheus data source
# 4. Import
```

### 2. Create Custom Dashboard

#### Service Overview Panel
```promql
# Service Status
ntp_up

# Response Time
histogram_quantile(0.95, rate(ntp_response_time_seconds_bucket[5m]))

# Request Rate
rate(ntp_requests_total[5m])

# Error Rate
rate(ntp_errors_total[5m])
```

#### Performance Panel
```promql
# Active Connections
ntp_active_connections

# Memory Usage
ntp_memory_bytes / ntp_memory_limit_bytes * 100

# CPU Usage
rate(ntp_cpu_seconds_total[5m]) * 100
```

#### Operational Panel
```promql
# Sync Status
ntp_sync_status

# Stratum Level
ntp_stratum

# Upstream Server Status
ntp_upstream_server_status
```

## Alerting Configuration

### 1. Install Alertmanager

```bash
# Download Alertmanager
wget https://github.com/prometheus/alertmanager/releases/download/v0.25.0/alertmanager-0.25.0.linux-amd64.tar.gz
tar xvf alertmanager-*.tar.gz
cd alertmanager-*

# Create configuration
cat > alertmanager.yml << EOF
global:
  smtp_smarthost: 'localhost:587'
  smtp_from: 'alertmanager@example.com'

route:
  group_by: ['alertname']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 1h
  receiver: 'team-ntp'

receivers:
  - name: 'team-ntp'
    email_configs:
      - to: 'ntp-team@example.com'
EOF

# Start Alertmanager
./alertmanager --config.file=alertmanager.yml
```

### 2. Configure Prometheus Alerting

```yaml
# Add to prometheus.yml
alerting:
  alertmanagers:
    - static_configs:
        - targets:
          - localhost:9093

rule_files:
  - "simple-ntpd-rules.yml"
```

### 3. Test Alerts

```bash
# Trigger test alert
curl -X POST http://localhost:9093/api/v1/alerts -d '[
  {
    "labels": {
      "alertname": "TestAlert",
      "severity": "warning"
    },
    "annotations": {
      "summary": "Test alert",
      "description": "This is a test alert"
    }
  }
]'
```

## Log Monitoring

### 1. Structured Logging

Configure simple-ntpd for structured logging:

```ini
[logging]
level = "INFO"
output = "json"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"
log_format = "json"
```

### 2. Log Aggregation

#### Using rsyslog

```bash
# Add to /etc/rsyslog.d/simple-ntpd.conf
local0.* /var/log/simple-ntpd/simple-ntpd.log
local0.* @log-server:514
```

#### Using systemd-journald

```bash
# View logs
sudo journalctl -u simple-ntpd -f

# Export logs
sudo journalctl -u simple-ntpd --since="1 hour ago" -o json > ntp-logs.json
```

### 3. Log Analysis

#### Using jq for JSON logs

```bash
# Filter error logs
cat /var/log/simple-ntpd/simple-ntpd.log | jq 'select(.level == "ERROR")'

# Count errors by type
cat /var/log/simple-ntpd/simple-ntpd.log | jq -r '.error_type' | sort | uniq -c

# Time-based analysis
cat /var/log/simple-ntpd/simple-ntpd.log | jq 'select(.timestamp | contains("2024-01-01"))'
```

## Performance Monitoring

### 1. Key Performance Indicators (KPIs)

- **Response Time**: 95th percentile < 100ms
- **Throughput**: Requests per second
- **Error Rate**: < 0.1% of requests
- **Availability**: > 99.9%
- **Resource Usage**: CPU < 80%, Memory < 80%

### 2. Performance Testing

```bash
# Load testing with ntpdate
for i in {1..100}; do
  ntpdate -q localhost &
done
wait

# Response time testing
time ntpdate -q localhost

# Concurrent connection testing
for i in {1..50}; do
  (echo "time"; sleep 1; echo "quit") | telnet localhost 123 &
done
```

### 3. Performance Tuning

#### System Tuning

```bash
# Increase UDP buffer sizes
echo 'net.core.rmem_max=26214400' >> /etc/sysctl.conf
echo 'net.core.wmem_max=26214400' >> /etc/sysctl.conf
sysctl -p

# Optimize network settings
echo 'net.core.netdev_max_backlog=5000' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_max_syn_backlog=5000' >> /etc/sysctl.conf
sysctl -p
```

#### Application Tuning

```ini
[performance]
worker_threads = 8
max_packet_size = 2048
connection_timeout = 30
enable_connection_pooling = true
max_pool_size = 1000
```

## Security Monitoring

### 1. Access Monitoring

```bash
# Monitor NTP access
sudo tcpdump -i any -n udp port 123

# Log access attempts
sudo iptables -A INPUT -p udp --dport 123 -j LOG --log-prefix "NTP-ACCESS: "
```

### 2. Anomaly Detection

```promql
# Unusual request patterns
rate(ntp_requests_total[5m]) > avg_over_time(ntp_requests_total[1h]) * 2

# Geographic anomalies (if geoip available)
ntp_requests_total{country!="US"} > 100
```

### 3. Security Alerts

```yaml
# Add to alerting rules
- alert: NTPDDoS
  expr: rate(ntp_requests_total[1m]) > 1000
  for: 1m
  labels:
    severity: critical
    type: security
  annotations:
    summary: "Potential NTP DDoS attack detected"
    description: "NTP request rate is {{ $value }} requests/minute"
```

## Troubleshooting

### 1. Common Issues

#### Metrics Not Scraping
```bash
# Check Prometheus targets
curl http://localhost:9090/api/v1/targets

# Check simple-ntpd metrics endpoint
curl http://localhost:8080/metrics

# Check firewall rules
sudo iptables -L | grep 8080
```

#### High Alert Volume
```bash
# Check alert status
curl http://localhost:9093/api/v1/alerts

# Silence noisy alerts
curl -X POST http://localhost:9093/api/v1/silences -d '{
  "matchers": [{"name": "alertname", "value": "NoisyAlert"}],
  "startsAt": "2024-01-01T00:00:00Z",
  "endsAt": "2024-01-01T23:59:59Z"
}'
```

### 2. Debug Commands

```bash
# Check Prometheus configuration
./prometheus --config.file=prometheus.yml --check-config

# Validate alerting rules
./promtool check rules simple-ntpd-rules.yml

# Test alerting rules
./promtool test rules test.yml
```

### 3. Performance Investigation

```bash
# Profile simple-ntpd
sudo perf record -g -p $(pgrep simple-ntpd)
sudo perf report

# Check system resources
htop
iotop
nethogs
```

## Integration Examples

### 1. Slack Integration

```yaml
# Add to alertmanager.yml
receivers:
  - name: 'slack-ntp'
    slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#ntp-alerts'
        title: 'NTP Alert: {{ .GroupLabels.alertname }}'
        text: '{{ range .Alerts }}{{ .Annotations.summary }}{{ end }}'
```

### 2. PagerDuty Integration

```yaml
# Add to alertmanager.yml
receivers:
  - name: 'pagerduty-ntp'
    pagerduty_configs:
      - routing_key: 'YOUR_PAGERDUTY_KEY'
        description: '{{ .GroupLabels.alertname }}'
        severity: '{{ if eq .GroupLabels.severity "critical" }}critical{{ else }}warning{{ end }}'
```

### 3. Email Integration

```yaml
# Add to alertmanager.yml
receivers:
  - name: 'email-ntp'
    email_configs:
      - to: 'ntp-team@example.com'
        send_resolved: true
        headers:
          subject: 'NTP Alert: {{ .GroupLabels.alertname }}'
```

## Best Practices

### 1. Monitoring Strategy

- **Start Simple**: Begin with basic availability and performance metrics
- **Iterate**: Add more detailed metrics based on operational needs
- **Document**: Maintain runbooks for common issues and resolutions
- **Test**: Regularly test alerting and notification systems

### 2. Alert Design

- **Actionable**: Alerts should trigger specific actions
- **Timely**: Set appropriate thresholds and durations
- **Informative**: Include context and runbook links
- **Tiered**: Use severity levels appropriately

### 3. Performance Considerations

- **Scrape Intervals**: Balance detail with resource usage
- **Retention**: Configure appropriate data retention periods
- **Cardinality**: Limit label cardinality to prevent explosion
- **Recording Rules**: Use recording rules for expensive queries

## Next Steps

1. **Custom Metrics**: Add application-specific metrics
2. **Advanced Alerting**: Implement complex alerting logic
3. **Capacity Planning**: Use metrics for capacity planning
4. **Automation**: Automate responses to common issues
5. **Compliance**: Ensure monitoring meets compliance requirements

## Resources

- [Prometheus Documentation](https://prometheus.io/docs/)
- [Grafana Documentation](https://grafana.com/docs/)
- [Alertmanager Documentation](https://prometheus.io/docs/alerting/latest/alertmanager/)
- [Monitoring Best Practices](https://prometheus.io/docs/practices/)
