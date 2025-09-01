#!/bin/bash
# Simple NTP Daemon - macOS Build Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="Release"
JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Building simple-ntpd on macOS..."

# Check prerequisites
if ! xcode-select -p &>/dev/null; then
    echo "ERROR: Xcode Command Line Tools not found"
    echo "Run: xcode-select --install"
    exit 1
fi

if ! command -v cmake &>/dev/null; then
    echo "ERROR: CMake not found"
    echo "Install via: brew install cmake"
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
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" \
    -DCMAKE_CXX_STANDARD=17

echo "Building with $JOBS jobs..."
cmake --build . --parallel "$JOBS"

echo "Build completed successfully!"
