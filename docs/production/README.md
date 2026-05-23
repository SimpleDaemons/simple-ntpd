# Production Version Documentation

**Version:** 1.0.0  
**License:** Apache 2.0  
**Status:** Production release

---

## Overview

The Production Version of Simple NTP Daemon is a complete NTP server for small to medium deployments, with upstream synchronization, security controls, and operational tooling.

## What's Included

- Complete NTP protocol server (RFC 5905 packet handling)
- Upstream time synchronization against configured servers
- High-performance UDP request handling
- Authentication, ACL, and rate limiting
- Multi-format configuration (JSON, YAML, INI) with hot reload
- Structured logging, Prometheus metrics, and health checks
- Cross-platform support (Linux, macOS; Windows build supported)

## Quick Commands

```bash
simple-ntpd start --config /etc/simple-ntpd/simple-ntpd.conf
simple-ntpd status
simple-ntpd health
simple-ntpd metrics
```

---

*Last Updated: May 2026*
