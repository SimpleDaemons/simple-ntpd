#!/bin/bash
# Simple NTP Daemon - Debian/Ubuntu Build Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="Release"
JOBS=$(nproc 2>/dev/null || echo 4)

echo "Building simple-ntpd on Debian/Ubuntu..."

# Check if we're on a Debian-based system
if ! command -v apt-get &>/dev/null; then
    echo "ERROR: This script is designed for Debian/Ubuntu systems"
    echo "Use build-linux.sh for other distributions"
    exit 1
fi

# Check prerequisites
if ! command -v cmake &>/dev/null; then
    echo "Installing CMake..."
    sudo apt-get update
    sudo apt-get install -y cmake
fi

if ! command -v g++ &>/dev/null; then
    echo "Installing build tools..."
    sudo apt-get update
    sudo apt-get install -y build-essential
fi

# Install additional dependencies
echo "Installing additional dependencies..."
sudo apt-get update
sudo apt-get install -y \
    libssl-dev \
    libsystemd-dev \
    pkg-config \
    check \
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
