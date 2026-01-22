# Deployment Examples

This document provides comprehensive deployment examples for Simple NTP Daemon across different environments and platforms.

## Deployment Overview

### Available Deployment Methods

1. **Bare Metal**: Direct installation on physical hardware
2. **Virtual Machine**: Deployment on virtualized infrastructure
3. **Docker**: Containerized deployment
4. **Kubernetes**: Orchestrated container deployment
5. **Cloud**: Cloud provider specific deployments

### Deployment Complexity Levels

- **Simple**: Single instance, basic configuration
- **Standard**: Multiple instances, load balancing
- **Advanced**: High availability, geographic distribution
- **Enterprise**: Multi-datacenter, advanced monitoring

## Simple Deployments

### 1. Single Server Deployment

**Use Case**: Development, testing, small office/home office (SOHO)

**Architecture**:
```
                    Internet
                         │
                    Firewall
                         │
                    NTP Server
                    (Stratum 2)
                         │
                    Upstream NTP
                    (Stratum 1)
```

**Deployment Steps**:
```bash
# 1. Install Simple NTP Daemon
git clone https://github.com/your-repo/simple-ntpd.git
cd simple-ntpd
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install

# 2. Create service user
sudo groupadd -r ntp
sudo useradd -r -g ntp -s /bin/false -d /var/lib/simple-ntpd ntp

# 3. Create directories
sudo mkdir -p /etc/simple-ntpd /var/log/simple-ntpd /var/lib/simple-ntpd

# 4. Configure service
sudo cp ../deployment/systemd/*.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable simple-ntpd
sudo systemctl start simple-ntpd

# 5. Configure firewall
sudo ufw allow 123/udp
```

**Configuration**:
```ini
[network]
listen_address = 0.0.0.0
listen_port = 123

[ntp_server]
stratum = 2
upstream_servers = ["pool.ntp.org", "time.nist.gov"]

[logging]
level = "INFO"
output = "file"
log_file = "/var/log/simple-ntpd/simple-ntpd.log"

[security]
enable_authentication = false
allow_query_from = ["0.0.0.0/0"]

[performance]
worker_threads = 4
max_packet_size = 1024
```

### 2. Docker Deployment

**Use Case**: Development, testing, containerized environments

**Architecture**:
```
                    Docker Host
                         │
                    Docker Engine
                         │
                    Simple NTPD
                    Container
                         │
                    Host Network
```

**Deployment Steps**:
```bash
# 1. Build Docker image
docker build -t simple-ntpd .

# 2. Run container
docker run -d \
  --name simple-ntpd \
  -p 123:123/udp \
  -v $(pwd)/config:/etc/simple-ntpd:ro \
  simple-ntpd

# 3. Verify deployment
docker ps
docker logs simple-ntpd
ntpdate -q localhost
```

**Docker Compose**:
```yaml
version: '3.8'

services:
  simple-ntpd:
    build: .
    container_name: simple-ntpd
    restart: unless-stopped
    ports:
      - "123:123/udp"
    volumes:
      - ./config:/etc/simple-ntpd:ro
      - ./logs:/var/log/simple-ntpd
    environment:
      - SIMPLE_NTPD_STRATUM=2
      - SIMPLE_NTPD_LOG_LEVEL=INFO
    networks:
      - ntp-network

networks:
  ntp-network:
    driver: bridge
```

## Standard Deployments

### 3. Multi-Instance Deployment

**Use Case**: Medium environments, load distribution, redundancy

**Architecture**:
```
                    Load Balancer
                         │
        ┌────────────────┼────────────────┐
        │                │                │
   NTP Server 1    NTP Server 2    NTP Server 3
   (Stratum 2)     (Stratum 2)     (Stratum 2)
        │                │                │
        └────────────────┼────────────────┘
                         │
                    Upstream NTP
                    (Stratum 1)
```

**Deployment Steps**:
```bash
# 1. Deploy multiple instances
for i in {1..3}; do
  ssh ntp-server-$i "git clone https://github.com/your-repo/simple-ntpd.git"
  ssh ntp-server-$i "cd simple-ntpd && ./scripts/install.sh --production"
done

# 2. Configure load balancer (HAProxy)
sudo apt install haproxy
sudo cp haproxy.cfg /etc/haproxy/haproxy.cfg
sudo systemctl restart haproxy
```

**HAProxy Configuration**:
```conf
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
```

### 4. Kubernetes Deployment

**Use Case**: Cloud-native environments, microservices architectures

**Architecture**:
```
                    Kubernetes Cluster
                         │
                    Load Balancer
                         │
                    NTP Service
                         │
        ┌────────────────┼────────────────┐
        │                │                │
   NTP Pod 1        NTP Pod 2        NTP Pod 3
   (Stratum 2)      (Stratum 2)      (Stratum 2)
        │                │                │
        └────────────────┼────────────────┘
                         │
                    Upstream NTP
                    (Stratum 1)
```

**Deployment Steps**:
```bash
# 1. Create namespace
kubectl apply -f namespace.yaml

# 2. Create service account
kubectl apply -f serviceaccount.yaml

# 3. Create ConfigMap
kubectl apply -f configmap.yaml

# 4. Deploy application
kubectl apply -f deployment.yaml

# 5. Create service
kubectl apply -f service.yaml

# 6. Verify deployment
kubectl get all -n ntp
kubectl logs -f deployment/simple-ntpd -n ntp
```

**Kubernetes Manifests**:

**Namespace**:
```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: ntp
  labels:
    name: ntp
    purpose: ntp-service
```

**Service Account**:
```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: simple-ntpd-sa
  namespace: ntp
```

**ConfigMap**:
```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: simple-ntpd-config
  namespace: ntp
data:
  simple-ntpd.conf: |
    [network]
    listen_address = 0.0.0.0
    listen_port = 123

    [ntp_server]
    stratum = 2
    upstream_servers = ["pool.ntp.org", "time.nist.gov"]

    [logging]
    level = "INFO"
    output = "file"
```

**Deployment**:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: simple-ntpd
  namespace: ntp
spec:
  replicas: 3
  selector:
    matchLabels:
      app: simple-ntpd
  template:
    metadata:
      labels:
        app: simple-ntpd
    spec:
      serviceAccountName: simple-ntpd-sa
      containers:
      - name: simple-ntpd
        image: simple-ntpd:latest
        ports:
        - containerPort: 123
          protocol: UDP
        volumeMounts:
        - name: config
          mountPath: /etc/simple-ntpd
          readOnly: true
      volumes:
      - name: config
        configMap:
          name: simple-ntpd-config
```

**Service**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: simple-ntpd-service
  namespace: ntp
spec:
  type: LoadBalancer
  ports:
  - port: 123
    targetPort: 123
    protocol: UDP
  selector:
    app: simple-ntpd
```

## Advanced Deployments

### 5. High Availability Deployment

**Use Case**: Enterprise environments, critical infrastructure

**Architecture**:
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

**Deployment Steps**:
```bash
# 1. Deploy to multiple data centers
for dc in dc1 dc2 dc3; do
  for node in {1..3}; do
    ssh $dc-ntp-$node "git clone https://github.com/your-repo/simple-ntpd.git"
    ssh $dc-ntp-$node "cd simple-ntpd && ./scripts/install.sh --production --high-availability"
  done
done

# 2. Configure global load balancer
# 3. Set up health checks
# 4. Configure failover
# 5. Test disaster recovery
```

**Load Balancer Configuration**:
```yaml
# AWS Application Load Balancer
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

### 6. Geographic Distribution

**Use Case**: Global applications, low-latency requirements

**Architecture**:
```
                    Route 53 (DNS)
                         │
        ┌────────────────┼────────────────┐
        │                │                │
   US East (N. Virginia)    Europe (Ireland)    Asia Pacific (Tokyo)
        │                │                │
   ┌────┴────┐      ┌────┴────┐      ┌────┴────┐
   │NTP Pool │      │NTP Pool │      │NTP Pool │
   │(3 nodes)│      │(3 nodes)│      │(3 nodes)│
   └─────────┘      └─────────┘      └─────────┘
```

**Deployment Steps**:
```bash
# 1. Deploy to multiple regions
for region in us-east-1 eu-west-1 ap-northeast-1; do
  aws eks update-kubeconfig --region $region --name ntp-cluster-$region

  kubectl apply -f namespace.yaml
  kubectl apply -f serviceaccount.yaml
  kubectl apply -f configmap.yaml
  kubectl apply -f deployment.yaml
  kubectl apply -f service.yaml
done

# 2. Configure Route 53
# 3. Set up health checks
# 4. Configure latency-based routing
```

**Route 53 Configuration**:
```bash
# Create latency-based routing
aws route53 create-hosted-zone --name ntp.example.com --caller-reference $(date +%s)

# Add latency records
aws route53 change-resource-record-sets \
  --hosted-zone-id Z1234567890 \
  --change-batch '{
    "Changes": [
      {
        "Action": "CREATE",
        "ResourceRecordSet": {
          "Name": "ntp.example.com",
          "Type": "A",
          "SetIdentifier": "us-east-1",
          "Region": "us-east-1",
          "TTL": 300,
          "ResourceRecords": [
            {"Value": "10.0.1.100"}
          ]
        }
      }
    ]
  }'
```

## Cloud-Specific Deployments

### 7. AWS Deployment

**Use Case**: AWS-based infrastructure, managed services

**Architecture**:
```
                    AWS Route 53
                         │
                    Application Load Balancer
                         │
                    EKS Cluster
                         │
                    Simple NTPD Pods
                         │
                    Upstream NTP
```

**Deployment Steps**:
```bash
# 1. Create EKS cluster
eksctl create cluster \
  --name ntp-cluster \
  --region us-east-1 \
  --nodegroup-name ntp-nodes \
  --node-type t3.medium \
  --nodes 3 \
  --nodes-min 3 \
  --nodes-max 6

# 2. Deploy Simple NTPD
kubectl apply -f k8s/

# 3. Configure ALB
kubectl apply -f alb-ingress.yaml

# 4. Set up monitoring
kubectl apply -f prometheus/
kubectl apply -f grafana/
```

**AWS-Specific Configuration**:
```yaml
# ALB Ingress Controller
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: ntp-ingress
  annotations:
    kubernetes.io/ingress.class: alb
    alb.ingress.kubernetes.io/scheme: internet-facing
    alb.ingress.kubernetes.io/target-type: ip
spec:
  rules:
  - http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: simple-ntpd-service
            port:
              number: 123
```

### 8. Azure Deployment

**Use Case**: Azure-based infrastructure, hybrid cloud

**Architecture**:
```
                    Azure DNS
                         │
                    Azure Load Balancer
                         │
                    AKS Cluster
                         │
                    Simple NTPD Pods
                         │
                    Upstream NTP
```

**Deployment Steps**:
```bash
# 1. Create AKS cluster
az aks create \
  --resource-group ntp-rg \
  --name ntp-cluster \
  --node-count 3 \
  --enable-addons monitoring \
  --generate-ssh-keys

# 2. Get credentials
az aks get-credentials --resource-group ntp-rg --name ntp-cluster

# 3. Deploy Simple NTPD
kubectl apply -f k8s/

# 4. Configure Azure Load Balancer
kubectl apply -f azure-lb.yaml
```

### 9. Google Cloud Deployment

**Use Case**: GCP-based infrastructure, global load balancing

**Architecture**:
```
                    Cloud DNS
                         │
                    Cloud Load Balancer
                         │
                    GKE Cluster
                         │
                    Simple NTPD Pods
                         │
                    Upstream NTP
```

**Deployment Steps**:
```bash
# 1. Create GKE cluster
gcloud container clusters create ntp-cluster \
  --zone us-central1-a \
  --num-nodes 3 \
  --enable-autoscaling \
  --min-nodes 3 \
  --max-nodes 6

# 2. Get credentials
gcloud container clusters get-credentials ntp-cluster --zone us-central1-a

# 3. Deploy Simple NTPD
kubectl apply -f k8s/

# 4. Configure Cloud Load Balancer
kubectl apply -f gcp-lb.yaml
```

## Monitoring and Observability

### 1. Prometheus Configuration

**Basic Prometheus Setup**:
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
    scrape_interval: 10s
    metrics_path: '/metrics'
```

**Advanced Prometheus Setup**:
```yaml
# prometheus-production.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    environment: 'production'
    datacenter: 'dc1'

scrape_configs:
  - job_name: 'simple-ntpd'
    static_configs:
      - targets: ['ntp1:8080', 'ntp2:8080', 'ntp3:8080']
        labels:
          service: 'simple-ntpd'
          environment: 'production'
          datacenter: 'dc1'
    scrape_interval: 10s
    metrics_path: '/metrics'
    basic_auth:
      username: 'monitoring'
      password: 'your-password'
```

### 2. Grafana Dashboards

**Basic Dashboard**:
```json
{
  "dashboard": {
    "title": "Simple NTPD Overview",
    "panels": [
      {
        "title": "Service Status",
        "type": "stat",
        "targets": [
          {
            "expr": "ntp_up",
            "legendFormat": "{{instance}}"
          }
        ]
      },
      {
        "title": "Response Time",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(ntp_response_time_seconds_bucket[5m]))",
            "legendFormat": "95th percentile"
          }
        ]
      }
    ]
  }
}
```

### 3. Alerting Rules

**Basic Alerting**:
```yaml
# simple-ntpd-rules.yml
groups:
  - name: simple-ntpd
    rules:
      - alert: NTPServiceDown
        expr: up{job="simple-ntpd"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "NTP service is down on {{ $labels.instance }}"
          description: "Simple NTP Daemon service has been down for more than 1 minute"
```

**Advanced Alerting**:
```yaml
# production-rules.yml
groups:
  - name: production-ntp
    rules:
      - alert: NTPServiceDown
        expr: up{job="simple-ntpd", environment="production"} == 0
        for: 1m
        labels:
          severity: critical
          environment: production
        annotations:
          summary: "NTP service is down in production on {{ $labels.instance }}"
          description: "Simple NTP Daemon service has been down for more than 1 minute"
          runbook_url: "https://wiki.example.com/runbooks/ntp-service-down"

      - alert: NTPHighResponseTime
        expr: histogram_quantile(0.95, rate(ntp_response_time_seconds_bucket[5m])) > 0.1
        for: 5m
        labels:
          severity: warning
          environment: production
        annotations:
          summary: "High NTP response time on {{ $labels.instance }}"
          description: "NTP response time is {{ $value }}s, which is above the threshold"
```

## Deployment Best Practices

### 1. **Planning and Design**
- Start with simple deployments and add complexity gradually
- Design for failure and implement redundancy
- Plan for growth and scaling
- Document your architecture and decisions

### 2. **Security**
- Implement network-level access controls
- Use appropriate authentication and authorization
- Enable rate limiting and DDoS protection
- Regular security updates and patches

### 3. **Performance**
- Monitor and optimize resource usage
- Implement appropriate caching strategies
- Use load balancing for distribution
- Regular performance testing and tuning

### 4. **Monitoring**
- Implement comprehensive monitoring
- Set up appropriate alerting
- Use structured logging
- Regular health checks and validation

### 5. **Maintenance**
- Establish regular maintenance procedures
- Implement automated backup and recovery
- Plan for updates and upgrades
- Document all procedures and configurations

## Troubleshooting

### 1. **Common Deployment Issues**
- Configuration errors
- Network connectivity problems
- Resource constraints
- Service dependency issues

### 2. **Debugging Steps**
- Check service status and logs
- Verify configuration files
- Test network connectivity
- Monitor resource usage

### 3. **Getting Help**
- Check the troubleshooting guide
- Review community resources
- Create detailed issue reports
- Engage with the community

## Conclusion

These deployment examples provide comprehensive guidance for deploying Simple NTP Daemon across different environments and platforms. Choose the approach that best fits your requirements and infrastructure.

Remember to:
- Start simple and add complexity as needed
- Implement appropriate monitoring and alerting
- Follow security best practices
- Document your deployment and procedures
- Test thoroughly before production use

For additional guidance and support, refer to the main documentation and community resources.
