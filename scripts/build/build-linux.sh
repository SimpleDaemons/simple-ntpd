#!/bin/bash
# Simple NTP Daemon - Generic Linux Build Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="Release"
JOBS=$(nproc 2>/dev/null || echo 4)

echo "Building simple-ntpd on Linux..."

# Check prerequisites
if ! command -v cmake &>/dev/null; then
    echo "ERROR: CMake not found"
    echo "Install via: sudo apt-get install cmake (Debian/Ubuntu)"
    echo "           or sudo yum install cmake (RHEL/CentOS)"
    exit 1
fi

if ! command -v g++ &>/dev/null; then
    echo "ERROR: C++ compiler not found"
    echo "Install via: sudo apt-get install build-essential (Debian/Ubuntu)"
    echo "           or sudo yum groupinstall 'Development Tools' (RHEL/CentOS)"
    exit 1
fi

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure and build
echo "Configuring build..."
cmake "$PROJECT_ROOT" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_CXX_STANDARD=17

echo "Building with $JOBS jobs..."
cmake --build . --parallel "$JOBS"

echo "Build completed successfully!"
