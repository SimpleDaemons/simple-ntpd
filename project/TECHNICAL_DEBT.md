# Simple NTP Daemon - Technical Debt

**Date:** May 2026  
**Current Version:** 1.0.0  
**Purpose:** Track technical debt, known issues, and areas requiring improvement

---

## Overview

Items below are prioritized by impact. Several v0.3.0 targets were addressed in v1.0.0; remaining work focuses on scale testing, documentation depth, and enterprise features.

**Estimated remaining effort:** ~80–120 hours (excluding v0.4.0 enterprise scope)

---

## High Priority

### 1. Test coverage expansion
**Status:** In progress  
**Estimated effort:** 25–35 hours

**Current state:**
- Eight CTest suites passing (unit, integration, UDP, security, performance smoke)
- Code coverage ~40–50% (estimated)
- No load/stress harness

**Action items:**
- [ ] Expand runtime tests for ACL/auth rejection paths
- [ ] Target 60%+ unit coverage
- [ ] Add soak test for long-running server stability
- [ ] Cross-platform CI matrix (Linux, macOS, Windows)

---

### 2. Load and performance benchmarking
**Status:** Partial (smoke tests only)  
**Estimated effort:** 15–20 hours

**Current state:**
- `test_ntp_performance` validates packet loop throughput with a loose ceiling
- No documented requests/sec capacity under concurrent clients

**Action items:**
- [ ] Define benchmark scenarios and acceptance thresholds
- [ ] Run load tests with realistic client concurrency
- [ ] Document results in `docs/production/performance.md`

---

### 3. Memory management review
**Status:** Needs review  
**Estimated effort:** 6–10 hours

**Action items:**
- [ ] Run AddressSanitizer builds in CI (`SIMPLE_NTPD_ENABLE_ASAN=ON`)
- [ ] Profile memory during extended uptime
- [ ] Verify connection/rate-limit map growth is bounded

---

## Medium Priority

### 4. Configuration schema completeness
**Status:** Partial  
**Estimated effort:** 8–12 hours

**Issues:**
- Example configs (especially `enterprise.conf`) contain keys not yet parsed
- Users may assume unsupported keys are active

**Action items:**
- [ ] Document supported vs. aspirational config keys
- [ ] Warn on unknown keys at parse time (optional strict mode)
- [ ] Align enterprise example with implemented options

---

### 5. Code and API documentation
**Status:** Partial  
**Estimated effort:** 10–15 hours

**Action items:**
- [ ] Complete header documentation for public interfaces
- [ ] Generate API reference (Doxygen or similar)
- [ ] Add troubleshooting guide for common production issues

---

### 6. Duplicate legacy source trees
**Status:** Open  
**Estimated effort:** 4–8 hours

**Issues:**
- Legacy mirrors exist under `src/core/` and `include/simple-ntpd/core/{config,ntp,utils}/`
- CMake builds only `src/simple-ntpd/` — duplicates cause confusion

**Action items:**
- [ ] Remove or clearly archive legacy mirrored sources
- [ ] Update contributor docs with canonical paths

---

## Low Priority

### 7. CLI polish
**Status:** Partial  
**Estimated effort:** 4–6 hours

**Issues:**
- Duplicate startup log lines from `main` and `NtpServer::start`
- `stop`/`restart` delegate to service manager messaging

**Action items:**
- [ ] Deduplicate startup logging
- [ ] Consider pid-file based stop/status for standalone deployments

---

### 8. Authentication model
**Status:** Functional but simplified  
**Estimated effort:** 20–30 hours (if full NTP MAC required)

**Notes:** Current auth uses OpenSSL digests over packet bytes; not interchangeable with ntpd/chrony key files.

---

## Resolved since v0.3.0

| Item | Resolution |
|------|------------|
| Upstream sync missing from repo | Gitignore fix + `upstream_sync` module committed |
| Upstream queries failed | RFC 5905 wire-format packet fix |
| Config reload loop on macOS | Filesystem mtime watcher fix |
| No UDP integration test | `test_ntp_udp` added |
| Security features "not implemented" | Runtime ACL/auth/rate-limit confirmed in audit |

---

## Next steps

1. Load/soak testing and documented capacity numbers
2. Config schema documentation and unknown-key warnings
3. Remove legacy source mirrors
4. Begin v0.4.0 enterprise planning (separate track)

---

*Last Updated: May 2026*  
*Next Review: v0.4.0 planning*
