# Simple NTP Daemon Windows Service Installer
# Copyright 2024 SimpleDaemons
# Licensed under Apache 2.0

param(
    [Parameter(Position=0)]
    [ValidateSet("install", "uninstall", "start", "stop", "restart", "status", "config", "help")]
    [string]$Action = "help",
    
    [Parameter()]
    [string]$ServiceName = "SimpleNtpd",
    
    [Parameter()]
    [string]$DisplayName = "Simple NTP Daemon",
    
    [Parameter()]
    [string]$Description = "Provides NTP time synchronization services",
    
    [Parameter()]
    [string]$BinaryPath = "$PSScriptRoot\simple-ntpd-service.exe",
    
    [Parameter()]
    [string]$ConfigDir = "$env:ProgramData\SimpleNtpd",
    
    [Parameter()]
    [string]$LogDir = "$env:ProgramData\SimpleNtpd\logs",
    
    [Parameter()]
    [string]$DataDir = "$env:ProgramData\SimpleNtpd\data"
)

# Set strict error handling
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Function to write colored output
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

# Function to check if running as administrator
function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Function to display usage
function Show-Usage {
    Write-ColorOutput "Simple NTP Daemon Windows Service Manager" "Cyan"
    Write-ColorOutput "=========================================" "Cyan"
    Write-ColorOutput ""
    Write-ColorOutput "Usage: $PSCommandPath [install|uninstall|start|stop|restart|status|config|help]" "Yellow"
    Write-ColorOutput ""
    Write-ColorOutput "Commands:" "White"
    Write-ColorOutput "  install   - Install the NTP service" "White"
    Write-ColorOutput "  uninstall - Remove the NTP service" "White"
    Write-ColorOutput "  start     - Start the NTP service" "White"
    Write-ColorOutput "  stop      - Stop the NTP service" "White"
    Write-ColorOutput "  restart   - Restart the NTP service" "White"
    Write-ColorOutput "  status    - Show service status" "White"
    Write-ColorOutput "  config    - Create configuration directory and files" "White"
    Write-ColorOutput "  help      - Show this help message" "White"
    Write-ColorOutput ""
    Write-ColorOutput "Parameters:" "White"
    Write-ColorOutput "  -ServiceName    - Service name (default: SimpleNtpd)" "White"
    Write-ColorOutput "  -DisplayName    - Service display name" "White"
    Write-ColorOutput "  -Description    - Service description" "White"
    Write-ColorOutput "  -BinaryPath     - Path to service binary" "White"
    Write-ColorOutput "  -ConfigDir      - Configuration directory" "White"
    Write-ColorOutput "  -LogDir         - Log directory" "White"
    Write-ColorOutput "  -DataDir        - Data directory" "White"
    Write-ColorOutput ""
}

# Function to install the service
function Install-Service {
    Write-ColorOutput "Installing Simple NTP Daemon service..." "Green"
    Write-ColorOutput ""
    
    # Check if binary exists
    if (-not (Test-Path $BinaryPath)) {
        Write-ColorOutput "ERROR: Service binary not found at $BinaryPath" "Red"
        Write-ColorOutput "Please build the service binary first" "Red"
        return 1
    }
    
    # Check if service already exists
    $existingService = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if ($existingService) {
        Write-ColorOutput "Service $ServiceName already exists" "Yellow"
        $choice = Read-Host "Do you want to reinstall it? (y/N)"
        if ($choice -eq "y" -or $choice -eq "Y") {
            Write-ColorOutput "Uninstalling existing service..." "Yellow"
            Uninstall-Service
        } else {
            Write-ColorOutput "Installation cancelled" "Yellow"
            return 0
        }
    }
    
    try {
        # Create service
        Write-ColorOutput "Creating service $ServiceName..." "White"
        $service = New-Service -Name $ServiceName -BinaryPathName $BinaryPath -DisplayName $DisplayName -StartupType Automatic -Description $Description
        
        # Set service dependencies
        $service | Set-Service -Dependencies "Tcpip"
        
        # Set service failure actions
        $service | Set-Service -StartupType Automatic
        
        Write-ColorOutput ""
        Write-ColorOutput "Service installed successfully!" "Green"
        Write-ColorOutput ""
        Write-ColorOutput "Next steps:" "White"
        Write-ColorOutput "1. Run '$PSCommandPath config' to create configuration files" "White"
        Write-ColorOutput "2. Run '$PSCommandPath start' to start the service" "White"
        Write-ColorOutput "3. Run '$PSCommandPath status' to check service status" "White"
        Write-ColorOutput ""
        
        return 0
    }
    catch {
        Write-ColorOutput "ERROR: Failed to create service" "Red"
        Write-ColorOutput $_.Exception.Message "Red"
        return 1
    }
}

# Function to uninstall the service
function Uninstall-Service {
    Write-ColorOutput "Uninstalling Simple NTP Daemon service..." "Green"
    Write-ColorOutput ""
    
    try {
        $service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
        if (-not $service) {
            Write-ColorOutput "Service $ServiceName not found" "Yellow"
            return 0
        }
        
        # Stop service if running
        if ($service.Status -eq "Running") {
            Write-ColorOutput "Stopping service..." "White"
            Stop-Service -Name $ServiceName -Force
            $service.WaitForStatus("Stopped", (New-TimeSpan -Seconds 30))
        }
        
        # Remove service
        Write-ColorOutput "Removing service..." "White"
        Remove-Service -Name $ServiceName
        
        Write-ColorOutput "Service removed successfully!" "Green"
        return 0
    }
    catch {
        Write-ColorOutput "ERROR: Failed to remove service" "Red"
        Write-ColorOutput $_.Exception.Message "Red"
        return 1
    }
}

# Function to start the service
function Start-Service {
    Write-ColorOutput "Starting Simple NTP Daemon service..." "Green"
    
    try {
        Start-Service -Name $ServiceName
        Write-ColorOutput "Service started successfully!" "Green"
        return 0
    }
    catch {
        Write-ColorOutput "ERROR: Failed to start service" "Red"
        Write-ColorOutput $_.Exception.Message "Red"
        Write-ColorOutput "Check the Windows Event Log for details" "Yellow"
        return 1
    }
}

# Function to stop the service
function Stop-Service {
    Write-ColorOutput "Stopping Simple NTP Daemon service..." "Green"
    
    try {
        Stop-Service -Name $ServiceName -Force
        Write-ColorOutput "Service stopped successfully!" "Green"
        return 0
    }
    catch {
        Write-ColorOutput "ERROR: Failed to stop service" "Red"
        Write-ColorOutput $_.Exception.Message "Red"
        return 1
    }
}

# Function to restart the service
function Restart-Service {
    Write-ColorOutput "Restarting Simple NTP Daemon service..." "Green"
    Stop-Service
    Start-Sleep -Seconds 2
    Start-Service
}

# Function to show service status
function Show-ServiceStatus {
    Write-ColorOutput "Simple NTP Daemon Service Status" "Cyan"
    Write-ColorOutput "================================" "Cyan"
    Write-ColorOutput ""
    
    try {
        Get-Service -Name $ServiceName | Format-List
        return 0
    }
    catch {
        Write-ColorOutput "ERROR: Failed to get service status" "Red"
        Write-ColorOutput $_.Exception.Message "Red"
        return 1
    }
}

# Function to create configuration
function Create-Configuration {
    Write-ColorOutput "Creating Simple NTP Daemon configuration..." "Green"
    Write-ColorOutput ""
    
    try {
        # Create directories
        Write-ColorOutput "Creating directories..." "White"
        New-Item -ItemType Directory -Path $ConfigDir -Force | Out-Null
        New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
        New-Item -ItemType Directory -Path $DataDir -Force | Out-Null
        
        # Create configuration file
        Write-ColorOutput "Creating configuration file..." "White"
        $configContent = @"
# Simple NTP Daemon Configuration File
# Generated on $(Get-Date)
#
# Network configuration
listen_address = 0.0.0.0
listen_port = 123
enable_ipv6 = true
max_connections = 1000
#
# NTP server configuration
stratum = 2
reference_clock = LOCAL
upstream_servers = pool.ntp.org,time.nist.gov
sync_interval = 64
timeout = 1000
#
# Logging configuration
log_level = 1
log_file = $LogDir\simple-ntpd.log
enable_console_logging = false
enable_syslog = false
#
# Security configuration
enable_authentication = false
authentication_key = 
restrict_queries = false
allowed_clients = 0.0.0.0/0
denied_clients = 
#
# Performance configuration
worker_threads = 4
max_packet_size = 1024
enable_statistics = true
stats_interval = 60
#
# Drift file configuration
drift_file = $DataDir\drift
enable_drift_compensation = true
#
# Leap second configuration
leap_second_file = $DataDir\leap-seconds.list
enable_leap_second_handling = true
"@
        
        $configContent | Out-File -FilePath "$ConfigDir\simple-ntpd.conf" -Encoding UTF8
        
        # Create log files
        Write-ColorOutput "Creating log files..." "White"
        "" | Out-File -FilePath "$LogDir\simple-ntpd.log" -Encoding UTF8
        "" | Out-File -FilePath "$LogDir\simple-ntpd.error.log" -Encoding UTF8
        
        # Set permissions (basic - in production you'd want more security)
        Write-ColorOutput "Setting permissions..." "White"
        $acl = Get-Acl $ConfigDir
        $rule = New-Object System.Security.AccessControl.FileSystemAccessRule("NT AUTHORITY\SYSTEM", "FullControl", "ContainerInherit,ObjectInherit", "None", "Allow")
        $acl.SetAccessRule($rule)
        Set-Acl $ConfigDir $acl
        
        $acl = Get-Acl $LogDir
        $acl.SetAccessRule($rule)
        Set-Acl $LogDir $acl
        
        $acl = Get-Acl $DataDir
        $acl.SetAccessRule($rule)
        Set-Acl $DataDir $acl
        
        Write-ColorOutput ""
        Write-ColorOutput "Configuration created successfully!" "Green"
        Write-ColorOutput ""
        Write-ColorOutput "Configuration file: $ConfigDir\simple-ntpd.conf" "White"
        Write-ColorOutput "Log directory: $LogDir" "White"
        Write-ColorOutput "Data directory: $DataDir" "White"
        Write-ColorOutput ""
        Write-ColorOutput "You can now edit the configuration file and start the service." "White"
        Write-ColorOutput ""
        
        return 0
    }
    catch {
        Write-ColorOutput "ERROR: Failed to create configuration" "Red"
        Write-ColorOutput $_.Exception.Message "Red"
        return 1
    }
}

# Main script logic
function Main {
    # Check if running as administrator
    if (-not (Test-Administrator)) {
        Write-ColorOutput "ERROR: This script must be run as Administrator" "Red"
        Write-ColorOutput "Right-click on PowerShell and select 'Run as administrator'" "Red"
        return 1
    }
    
    # Process action
    switch ($Action) {
        "install" { return Install-Service }
        "uninstall" { return Uninstall-Service }
        "start" { return Start-Service }
        "stop" { return Stop-Service }
        "restart" { return Restart-Service }
        "status" { return Show-ServiceStatus }
        "config" { return Create-Configuration }
        "help" { Show-Usage; return 0 }
        default { Show-Usage; return 1 }
    }
}

# Run main function
Main
