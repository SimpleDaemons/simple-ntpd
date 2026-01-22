# Build Guide

Complete guide to building Simple NTP Daemon for different versions and platforms.

## Quick Start

```bash
# Clone repository
git clone https://github.com/SimpleDaemons/simple-ntpd.git
cd simple-ntpd

# Build Production version
mkdir build && cd build
cmake -DBUILD_VERSION=production ..
make

# Run
./simple-ntpd --help
```

---

## Build Commands Reference

### Basic Build

```bash
# Configure
cmake ..

# Build
make

# Install (requires root)
sudo make install
```

### Version-Specific Builds

#### Production Version
```bash
cmake -DBUILD_VERSION=production ..
make
```

#### Enterprise Version
```bash
cmake -DBUILD_VERSION=enterprise ..
make
```

#### Datacenter Version
```bash
cmake -DBUILD_VERSION=datacenter ..
make
```

---

**Last Updated:** December 2024
