# Kubernetes Deployment Guide

This guide covers deploying simple-ntpd on Kubernetes clusters.

## Prerequisites

- Kubernetes cluster (v1.19+)
- `kubectl` configured to access your cluster
- `helm` (optional, for Helm deployment)

## Quick Start

### 1. Create Namespace

```bash
kubectl apply -f namespace.yaml
```

### 2. Create Service Account

```bash
kubectl apply -f serviceaccount.yaml
```

### 3. Create ConfigMap

```bash
kubectl apply -f configmap.yaml
```

### 4. Deploy Application

```bash
kubectl apply -f deployment.yaml
```

### 5. Create Service

```bash
kubectl apply -f service.yaml
```

### 6. Verify Deployment

```bash
# Check all resources
kubectl get all -n ntp

# Check pod status
kubectl get pods -n ntp

# Check service
kubectl get svc -n ntp

# Check logs
kubectl logs -f deployment/simple-ntpd -n ntp
```

## All-in-One Deployment

Deploy everything at once:

```bash
kubectl apply -f .
```

## Configuration

### Environment Variables

The deployment supports the following environment variables:

- `SIMPLE_NTPD_LISTEN_ADDRESS`: Bind address (default: 0.0.0.0)
- `SIMPLE_NTPD_LISTEN_PORT`: Bind port (default: 123)
- `SIMPLE_NTPD_STRATUM`: Stratum level (default: 2)
- `SIMPLE_NTPD_LOG_LEVEL`: Log level (default: INFO)

### Configuration File

The configuration is stored in a ConfigMap and mounted into the container. To modify the configuration:

1. Edit `configmap.yaml`
2. Apply the changes: `kubectl apply -f configmap.yaml`
3. Restart the deployment: `kubectl rollout restart deployment/simple-ntpd -n ntp`

## Production Deployment

### 1. Resource Requirements

Adjust resource limits based on your workload:

```yaml
resources:
  requests:
    memory: "256Mi"
    cpu: "200m"
  limits:
    memory: "512Mi"
    cpu: "1000m"
```

### 2. High Availability

The deployment includes:
- Multiple replicas (3 by default)
- Pod anti-affinity for distribution across nodes
- Liveness and readiness probes
- Graceful shutdown handling

### 3. Security

Security features:
- Non-root user execution
- Read-only configuration mounts
- Network policies (if configured)
- RBAC integration

## Monitoring and Observability

### Health Checks

The deployment includes health checks:

- **Liveness Probe**: Restarts pods if NTP service becomes unresponsive
- **Readiness Probe**: Ensures pods are ready to receive traffic

### Logging

Logs are available via:

```bash
# Follow logs from all pods
kubectl logs -f -l app=simple-ntpd -n ntp

# Get logs from specific pod
kubectl logs <pod-name> -n ntp

# Export logs
kubectl logs -l app=simple-ntpd -n ntp > ntp-logs.txt
```

### Metrics

For metrics collection, consider adding:

- Prometheus annotations
- Custom metrics endpoints
- Service mesh integration

## Scaling

### Horizontal Scaling

```bash
# Scale to 5 replicas
kubectl scale deployment simple-ntpd --replicas=5 -n ntp

# Auto-scaling (requires HPA)
kubectl autoscale deployment simple-ntpd --min=3 --max=10 --cpu-percent=70 -n ntp
```

### Vertical Scaling

Adjust resource limits in the deployment:

```yaml
resources:
  requests:
    memory: "512Mi"
    cpu: "500m"
  limits:
    memory: "1Gi"
    cpu: "2000m"
```

## Load Balancing

### Service Types

- **LoadBalancer**: External load balancer (cloud providers)
- **NodePort**: Direct node access
- **ClusterIP**: Internal cluster access

### Ingress (Optional)

For HTTP-based monitoring endpoints:

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: simple-ntpd-ingress
  namespace: ntp
spec:
  rules:
  - host: ntp.example.com
    http:
      paths:
      - path: /health
        pathType: Prefix
        backend:
          service:
            name: simple-ntpd-service
            port:
              number: 8080
```

## Backup and Recovery

### Configuration Backup

```bash
# Backup ConfigMap
kubectl get configmap simple-ntpd-config -n ntp -o yaml > config-backup.yaml

# Backup all resources
kubectl get all -n ntp -o yaml > ntp-backup.yaml
```

### Disaster Recovery

```bash
# Restore from backup
kubectl apply -f ntp-backup.yaml

# Or restore specific resources
kubectl apply -f config-backup.yaml
```

## Troubleshooting

### Common Issues

1. **Pods Not Starting**
   ```bash
   # Check pod events
   kubectl describe pod <pod-name> -n ntp

   # Check pod logs
   kubectl logs <pod-name> -n ntp
   ```

2. **Service Not Accessible**
   ```bash
   # Check service endpoints
   kubectl get endpoints -n ntp

   # Test connectivity
   kubectl run test-pod --image=busybox --rm -it --restart=Never -- nslookup simple-ntpd-service.ntp
   ```

3. **Configuration Issues**
   ```bash
   # Validate ConfigMap
   kubectl get configmap simple-ntpd-config -n ntp -o yaml

   # Check mounted configuration
   kubectl exec <pod-name> -n ntp -- cat /etc/simple-ntpd/simple-ntpd.conf
   ```

### Debug Commands

```bash
# Get detailed pod information
kubectl describe pod <pod-name> -n ntp

# Execute commands in pod
kubectl exec -it <pod-name> -n ntp -- /bin/sh

# Port forward for local testing
kubectl port-forward svc/simple-ntpd-service 123:123 -n ntp
```

## Performance Tuning

### Network Optimization

```yaml
# Add to deployment spec
spec:
  template:
    spec:
      containers:
      - name: simple-ntpd
        # ... existing config ...
        env:
        - name: SIMPLE_NTPD_WORKER_THREADS
          value: "8"
        - name: SIMPLE_NTPD_MAX_PACKET_SIZE
          value: "2048"
```

### Node Tuning

On worker nodes, consider:

```bash
# Increase UDP buffer sizes
echo 'net.core.rmem_max=26214400' >> /etc/sysctl.conf
echo 'net.core.wmem_max=26214400' >> /etc/sysctl.conf
sysctl -p
```

## Helm Deployment (Optional)

### Create Helm Chart

```bash
# Create chart structure
helm create simple-ntpd

# Copy manifests to templates/
cp *.yaml simple-ntpd/templates/

# Install chart
helm install simple-ntpd ./simple-ntpd -n ntp
```

### Helm Values

Customize deployment with values.yaml:

```yaml
replicaCount: 3
image:
  repository: simple-ntpd
  tag: latest
  pullPolicy: IfNotPresent

resources:
  limits:
    cpu: 500m
    memory: 256Mi
  requests:
    cpu: 100m
    memory: 128Mi

config:
  stratum: 2
  logLevel: INFO
```

## Security Best Practices

### Network Policies

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: simple-ntpd-network-policy
  namespace: ntp
spec:
  podSelector:
    matchLabels:
      app: simple-ntpd
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - ports:
    - protocol: UDP
      port: 123
  egress:
  - ports:
    - protocol: UDP
      port: 123
```

### RBAC

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: simple-ntpd-role
  namespace: ntp
rules:
- apiGroups: [""]
  resources: ["pods", "services"]
  verbs: ["get", "list", "watch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: simple-ntpd-rolebinding
  namespace: ntp
subjects:
- kind: ServiceAccount
  name: simple-ntpd-sa
  namespace: ntp
roleRef:
  kind: Role
  name: simple-ntpd-role
  apiGroup: rbac.authorization.k8s.io
```

## Support

For Kubernetes deployment issues:

1. Check pod status and logs
2. Verify resource availability
3. Check network policies and RBAC
4. Review cluster events
5. Test connectivity between components

Refer to the main deployment README for additional troubleshooting steps.
