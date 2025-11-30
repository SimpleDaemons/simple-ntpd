#!/bin/bash
# Post-installation script for simple-ntpd RPM

set -e

PROJECT_NAME="simple-ntpd"
SERVICE_USER="ntpddev"

# Create service user if it doesn't exist
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd -r -s /sbin/nologin -d /var/lib/$simple-ntpd -c "$simple-ntpd service user" "$SERVICE_USER"
fi

# Set ownership
chown -R "$SERVICE_USER:$SERVICE_USER" /etc/$simple-ntpd 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/log/$simple-ntpd 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/lib/$simple-ntpd 2>/dev/null || true

# Enable and start service
systemctl daemon-reload
systemctl enable "$simple-ntpd" 2>/dev/null || true
systemctl start "$simple-ntpd" 2>/dev/null || true

exit 0

