#!/bin/bash
# Post-uninstallation script for simple-ntpd RPM

set -e

# Reload systemd
systemctl daemon-reload

exit 0

