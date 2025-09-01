#!/bin/bash

# Docker-based cross-platform build script for Simple NTP Daemon

set -e

# Default values
DISTRO="all"
ARCH="x86_64"
CLEAN=false

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -d, --distro DISTRO    Build for specific distribution (ubuntu, centos, alpine, all)"
    echo "  -a, --arch ARCH        Target architecture (x86_64, arm64, armv7, all)"
    echo "  -c, --clean            Clean build cache before building"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -d ubuntu -a x86_64"
    echo "  $0 -d all -a all"
    echo "  $0 --clean"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--distro)
            DISTRO="$2"
            shift 2
            ;;
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Check prerequisites
echo "Checking prerequisites..."
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker is not installed"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo "ERROR: Docker Compose is not installed"
    exit 1
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning build cache..."
    docker-compose down -v
    docker system prune -f
fi

# Build based on options
echo "Starting build for $DISTRO ($ARCH)..."

if [ "$DISTRO" = "all" ]; then
    echo "Building for all distributions..."
    docker-compose --profile build build build-ubuntu build-centos build-alpine
else
    case $DISTRO in
        "ubuntu")
            docker-compose --profile build build build-ubuntu
            ;;
        "centos")
            docker-compose --profile build build build-centos
            ;;
        "alpine")
            docker-compose --profile build build build-alpine
            ;;
        *)
            echo "ERROR: Unsupported distribution: $DISTRO"
            exit 1
            ;;
    esac
fi

echo "Build completed successfully!"
echo "Artifacts available in ./dist/"
