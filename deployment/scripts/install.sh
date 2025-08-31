#!/bin/bash
#
# Simple NTP Daemon Installation Script
# Copyright 2024 BLBurns <contact@blburns.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

# Script configuration
SCRIPT_NAME=$(basename "$0")
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
PROJECT_NAME="simple-ntpd"
VERSION="0.1.0"

# Installation paths
INSTALL_DIR="/usr/local"
BIN_DIR="$INSTALL_DIR/bin"
LIB_DIR="$INSTALL_DIR/lib"
CONFIG_DIR="/etc/simple-ntpd"
LOG_DIR="/var/log/simple-ntpd"
DATA_DIR="/var/lib/simple-ntpd"
RUN_DIR="/var/run/simple-ntpd"
SERVICE_USER="ntp"
SERVICE_GROUP="ntp"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root"
        exit 1
    fi
}

# Function to detect operating system
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS="$NAME"
        VER="$VERSION_ID"
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    elif [[ -f /etc/lsb-release ]]; then
        . /etc/lsb-release
        OS="$DISTRIB_ID"
        VER="$DISTRIB_RELEASE"
    elif [[ -f /etc/debian_version ]]; then
        OS=Debian
        VER=$(cat /etc/debian_version)
    elif [[ -f /etc/SuSe-release ]]; then
        OS=SuSE
    elif [[ -f /etc/redhat-release ]]; then
        OS=RedHat
    else
        OS=$(uname -s)
        VER=$(uname -r)
    fi
    
    log_info "Detected OS: $OS $VER"
}

# Function to check dependencies
check_dependencies() {
    log_info "Checking system dependencies..."
    
    local missing_deps=()
    
    # Check for required commands
    for cmd in cmake make g++ ldconfig; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            missing_deps+=("$cmd")
        fi
    done
    
    # Check for required libraries
    if ! ldconfig -p | grep -q "libc.so"; then
        missing_deps+=("libc")
    fi
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_error "Missing dependencies: ${missing_deps[*]}"
        log_info "Please install the required packages and try again"
        exit 1
    fi
    
    log_success "All dependencies are satisfied"
}

# Function to create service user and group
create_service_user() {
    log_info "Creating service user and group..."
    
    # Create group if it doesn't exist
    if ! getent group "$SERVICE_GROUP" >/dev/null 2>&1; then
        groupadd -r "$SERVICE_GROUP"
        log_success "Created group: $SERVICE_GROUP"
    else
        log_info "Group $SERVICE_GROUP already exists"
    fi
    
    # Create user if it doesn't exist
    if ! getent passwd "$SERVICE_USER" >/dev/null 2>&1; then
        useradd -r -g "$SERVICE_GROUP" -s /bin/false -d /var/lib/simple-ntpd "$SERVICE_USER"
        log_success "Created user: $SERVICE_USER"
    else
        log_info "User $SERVICE_USER already exists"
    fi
}

# Function to create directories
create_directories() {
    log_info "Creating installation directories..."
    
    local dirs=("$BIN_DIR" "$LIB_DIR" "$CONFIG_DIR" "$LOG_DIR" "$DATA_DIR" "$RUN_DIR")
    
    for dir in "${dirs[@]}"; do
        if [[ ! -d "$dir" ]]; then
            mkdir -p "$dir"
            log_success "Created directory: $dir"
        else
            log_info "Directory already exists: $dir"
        fi
    done
    
    # Set proper permissions
    chown -R "$SERVICE_USER:$SERVICE_GROUP" "$LOG_DIR" "$DATA_DIR" "$RUN_DIR"
    chmod 755 "$LOG_DIR" "$DATA_DIR" "$RUN_DIR"
    chmod 755 "$CONFIG_DIR"
}

# Function to build the project
build_project() {
    log_info "Building $PROJECT_NAME..."
    
    cd "$PROJECT_ROOT"
    
    if [[ ! -d "build" ]]; then
        mkdir build
    fi
    
    cd build
    
    # Configure with CMake
    log_info "Configuring with CMake..."
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_TESTS=ON \
          -DBUILD_EXAMPLES=OFF \
          -DENABLE_LOGGING=ON \
          -DENABLE_IPV6=ON \
          ..
    
    # Build the project
    log_info "Building the project..."
    make -j$(nproc)
    
    log_success "Build completed successfully"
}

# Function to install the application
install_application() {
    log_info "Installing $PROJECT_NAME..."
    
    cd "$PROJECT_ROOT/build"
    
    # Install binaries and libraries
    make install DESTDIR=""
    
    # Install configuration files
    if [[ -f "$PROJECT_ROOT/config/simple-ntpd.conf.example" ]]; then
        cp "$PROJECT_ROOT/config/simple-ntpd.conf.example" "$CONFIG_DIR/simple-ntpd.conf"
        chown "$SERVICE_USER:$SERVICE_GROUP" "$CONFIG_DIR/simple-ntpd.conf"
        chmod 644 "$CONFIG_DIR/simple-ntpd.conf"
        log_success "Configuration file installed"
    fi
    
    log_success "Application installed successfully"
}

# Function to install systemd service
install_systemd_service() {
    log_info "Installing systemd service..."
    
    local service_file="$PROJECT_ROOT/deployment/systemd/simple-ntpd.service"
    local socket_file="$PROJECT_ROOT/deployment/systemd/simple-ntpd.socket"
    local target_file="$PROJECT_ROOT/deployment/systemd/simple-ntpd.target"
    
    if [[ -f "$service_file" ]]; then
        cp "$service_file" /etc/systemd/system/
        log_success "Systemd service file installed"
    fi
    
    if [[ -f "$socket_file" ]]; then
        cp "$socket_file" /etc/systemd/system/
        log_success "Systemd socket file installed"
    fi
    
    if [[ -f "$target_file" ]]; then
        cp "$target_file" /etc/systemd/system/
        log_success "Systemd target file installed"
    fi
    
    # Reload systemd and enable service
    systemctl daemon-reload
    systemctl enable simple-ntpd.socket
    systemctl enable simple-ntpd.service
    
    log_success "Systemd service installed and enabled"
}

# Function to install init.d script
install_initd_script() {
    log_info "Installing init.d script..."
    
    local initd_file="$PROJECT_ROOT/deployment/init.d/simple-ntpd"
    
    if [[ -f "$initd_file" ]]; then
        cp "$initd_file" /etc/init.d/
        chmod 755 /etc/init.d/simple-ntpd
        
        # Enable service based on OS
        if command -v chkconfig >/dev/null 2>&1; then
            chkconfig --add simple-ntpd
            chkconfig --level 2345 simple-ntpd on
        elif command -v update-rc.d >/dev/null 2>&1; then
            update-rc.d simple-ntpd defaults
        fi
        
        log_success "Init.d script installed and enabled"
    else
        log_warning "Init.d script not found, skipping"
    fi
}

# Function to configure firewall
configure_firewall() {
    log_info "Configuring firewall..."
    
    # Check if firewalld is running
    if systemctl is-active --quiet firewalld; then
        firewall-cmd --permanent --add-service=ntp
        firewall-cmd --reload
        log_success "Firewalld configured for NTP"
    # Check if ufw is active
    elif command -v ufw >/dev/null 2>&1 && ufw status | grep -q "Status: active"; then
        ufw allow 123/udp
        log_success "UFW configured for NTP"
    # Check if iptables is available
    elif command -v iptables >/dev/null 2>&1; then
        iptables -A INPUT -p udp --dport 123 -j ACCEPT
        log_success "IPTables configured for NTP"
    else
        log_warning "No firewall detected, manual configuration may be required"
    fi
}

# Function to set up log rotation
setup_log_rotation() {
    log_info "Setting up log rotation..."
    
    local logrotate_conf="/etc/logrotate.d/simple-ntpd"
    
    cat > "$logrotate_conf" << EOF
$LOG_DIR/*.log {
    daily
    missingok
    rotate 52
    compress
    delaycompress
    notifempty
    create 644 $SERVICE_USER $SERVICE_GROUP
    postrotate
        systemctl reload simple-ntpd > /dev/null 2>&1 || true
    endscript
}
EOF
    
    chmod 644 "$logrotate_conf"
    log_success "Log rotation configured"
}

# Function to verify installation
verify_installation() {
    log_info "Verifying installation..."
    
    local errors=0
    
    # Check if binary exists
    if [[ ! -f "$BIN_DIR/simple-ntpd" ]]; then
        log_error "Binary not found: $BIN_DIR/simple-ntpd"
        ((errors++))
    fi
    
    # Check if configuration exists
    if [[ ! -f "$CONFIG_DIR/simple-ntpd.conf" ]]; then
        log_error "Configuration not found: $CONFIG_DIR/simple-ntpd.conf"
        ((errors++))
    fi
    
    # Check if service user exists
    if ! getent passwd "$SERVICE_USER" >/dev/null 2>&1; then
        log_error "Service user not found: $SERVICE_USER"
        ((errors++))
    fi
    
    # Check if directories exist
    local dirs=("$LOG_DIR" "$DATA_DIR" "$RUN_DIR")
    for dir in "${dirs[@]}"; do
        if [[ ! -d "$dir" ]]; then
            log_error "Directory not found: $dir"
            ((errors++))
        fi
    done
    
    if [[ $errors -eq 0 ]]; then
        log_success "Installation verification passed"
        return 0
    else
        log_error "Installation verification failed with $errors errors"
        return 1
    fi
}

# Function to display post-installation information
display_post_install_info() {
    echo
    log_success "Installation completed successfully!"
    echo
    echo "Post-installation steps:"
    echo "1. Review configuration: $CONFIG_DIR/simple-ntpd.conf"
    echo "2. Start the service:"
    echo "   - Systemd: systemctl start simple-ntpd"
    echo "   - Init.d:  /etc/init.d/simple-ntpd start"
    echo "3. Check service status:"
    echo "   - Systemd: systemctl status simple-ntpd"
    echo "   - Init.d:  /etc/init.d/simple-ntpd status"
    echo "4. Test NTP service: ntpdate -q localhost"
    echo
    echo "Configuration file: $CONFIG_DIR/simple-ntpd.conf"
    echo "Log files: $LOG_DIR/"
    echo "Data directory: $DATA_DIR/"
    echo
    echo "For more information, see the README.md file"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $SCRIPT_NAME [OPTIONS]

Install Simple NTP Daemon

Options:
    -h, --help          Show this help message
    -v, --version       Show version information
    --skip-build        Skip building the project (assume pre-built)
    --skip-systemd      Skip systemd service installation
    --skip-initd        Skip init.d script installation
    --skip-firewall     Skip firewall configuration
    --skip-logrotate    Skip log rotation setup

Examples:
    $SCRIPT_NAME                    # Full installation
    $SCRIPT_NAME --skip-build       # Install without building
    $SCRIPT_NAME --skip-systemd     # Install without systemd

EOF
}

# Function to show version
show_version() {
    echo "$PROJECT_NAME $VERSION"
    echo "Installation script"
    echo "Copyright 2024 BLBurns <contact@blburns.com>"
}

# Main installation function
main() {
    local skip_build=false
    local skip_systemd=false
    local skip_initd=false
    local skip_firewall=false
    local skip_logrotate=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -v|--version)
                show_version
                exit 0
                ;;
            --skip-build)
                skip_build=true
                shift
                ;;
            --skip-systemd)
                skip_systemd=true
                shift
                ;;
            --skip-initd)
                skip_initd=true
                shift
                ;;
            --skip-firewall)
                skip_firewall=true
                shift
                ;;
            --skip-logrotate)
                skip_logrotate=true
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    log_info "Starting $PROJECT_NAME installation..."
    log_info "Version: $VERSION"
    log_info "Install directory: $INSTALL_DIR"
    
    # Check prerequisites
    check_root
    detect_os
    check_dependencies
    
    # Installation steps
    create_service_user
    create_directories
    
    if [[ "$skip_build" == false ]]; then
        build_project
    else
        log_info "Skipping build step"
    fi
    
    install_application
    
    if [[ "$skip_systemd" == false ]]; then
        install_systemd_service
    else
        log_info "Skipping systemd service installation"
    fi
    
    if [[ "$skip_initd" == false ]]; then
        install_initd_script
    else
        log_info "Skipping init.d script installation"
    fi
    
    if [[ "$skip_firewall" == false ]]; then
        configure_firewall
    else
        log_info "Skipping firewall configuration"
    fi
    
    if [[ "$skip_logrotate" == false ]]; then
        setup_log_rotation
    else
        log_info "Skipping log rotation setup"
    fi
    
    # Verify installation
    if verify_installation; then
        display_post_install_info
        log_success "Installation completed successfully!"
        exit 0
    else
        log_error "Installation failed verification"
        exit 1
    fi
}

# Run main function with all arguments
main "$@"
