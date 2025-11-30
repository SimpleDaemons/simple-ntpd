#!/bin/bash
# Pre-installation script for simple-ntpd RPM
# License acceptance check

set -e

PROJECT_NAME="simple-ntpd"

# Display license and require acceptance
echo "=========================================="
echo "simple-ntpd License Agreement"
echo "=========================================="
echo ""
cat /usr/share/doc/$simple-ntpd/LICENSE 2>/dev/null || \
    cat /usr/share/$simple-ntpd/LICENSE.txt 2>/dev/null || \
    echo "Please review the license at: https://github.com/simpledaemons/$simple-ntpd/blob/main/LICENSE"
echo ""
echo "By continuing, you agree to the terms of the license."
echo ""
read -p "Do you accept the license? [y/N]: " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "License not accepted. Installation cancelled."
    exit 1
fi

exit 0

