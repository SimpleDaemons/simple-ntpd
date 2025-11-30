#!/bin/bash
# Pre-uninstallation script for simple-ntpd RPM

set -e

PROJECT_NAME="simple-ntpd"

# Stop service before removal
if [ "$1" -eq 0 ]; then
    systemctl stop "$simple-ntpd" 2>/dev/null || true
    systemctl disable "$simple-ntpd" 2>/dev/null || true
fi

exit 0

