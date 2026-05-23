# Project Management Documentation

This directory contains project management, development, and internal documentation for the Simple NTP Daemon project.

**Current version:** v1.0.0 (Production release, May 2026)

## Document Organization

### Project Status & Progress
- **[PROJECT_STATUS.md](PROJECT_STATUS.md)** — Overall status, feature matrix, test commands
- **[PROGRESS_REPORT.md](PROGRESS_REPORT.md)** — Milestone history and current gaps
- **[DEVELOPMENT_SUMMARY.md](DEVELOPMENT_SUMMARY.md)** — Development timeline and accomplishments

### Implementation & Features
- **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** — v1.0.0 features and post-release hardening
- **[FEATURE_AUDIT.md](FEATURE_AUDIT.md)** — Implemented vs. planned feature audit

### Planning & Quality
- **[ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md)** — Version checklist and quality gates
- **[TECHNICAL_DEBT.md](TECHNICAL_DEBT.md)** — Known issues and remaining work
- **[ROADMAP.md](../ROADMAP.md)** — Public roadmap (project root)

## Quick reference

```bash
make test                    # 8 CTest suites
./build/simple-ntpd --version
./build/simple-ntpd health --config <file>
```

## User documentation

User-facing guides live in **[docs/](../docs/)**.

## Version history

See **[CHANGELOG.md](../CHANGELOG.md)** in the project root.

---

*Internal project management docs. Last updated: May 2026.*
