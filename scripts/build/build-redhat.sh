#!/bin/bash
# Simple NTP Daemon - RedHat/CentOS/Fedora Build Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="Release"
JOBS=$(nproc 2>/dev/null || echo 4)

echo "Building simple-ntpd on RedHat/CentOS/Fedora..."

# Check if we're on a RedHat-based system
if ! command -v yum &>/dev/null && ! command -v dnf &>/dev/null; then
    echo "ERROR: This script is designed for RedHat-based systems"
    echo "Use build-linux.sh for other distributions"
    exit 1
fi

# Determine package manager
if command -v dnf &>/dev/null; then
    PKG_MANAGER="dnf"
    PKG_GROUP="Development Tools"
else
    PKG_MANAGER="yum"
    PKG_GROUP="Development Tools"
fi

# Check prerequisites
if ! command -v cmake &>/dev/null; then
    echo "Installing CMake..."
    sudo $PKG_MANAGER install -y cmake
fi

if ! command -v g++ &>/dev/null; then
    echo "Installing build tools..."
    sudo $PKG_MANAGER groupinstall -y "$PKG_GROUP"
fi

# Install additional dependencies
echo "Installing additional dependencies..."
sudo $PKG_MANAGER install -y \
    openssl-devel \
    systemd-devel \
    pkgconfig \
    check-devel \
    valgrind

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure and build
echo "Configuring build..."
cmake "$PROJECT_ROOT" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_CXX_STANDARD=17 \
    -DBUILD_TESTS=ON \
    -DENABLE_SYSTEMD=ON

echo "Building with $JOBS jobs..."
cmake --build . --parallel "$JOBS"

# Run tests
echo "Running tests..."
ctest --output-on-failure

echo "Build completed successfully!"
