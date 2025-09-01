@echo off
REM Simple NTP Daemon Windows Service Installer
REM Copyright 2024 SimpleDaemons
REM Licensed under Apache 2.0

setlocal enabledelayedexpansion

REM Configuration
set SERVICE_NAME=SimpleNtpd
set SERVICE_DISPLAY_NAME=Simple NTP Daemon
set SERVICE_DESCRIPTION=Provides NTP time synchronization services
set BINARY_PATH=%~dp0simple-ntpd-service.exe
set CONFIG_DIR=%ProgramData%\SimpleNtpd
set LOG_DIR=%ProgramData%\SimpleNtpd\logs
set DATA_DIR=%ProgramData%\SimpleNtpd\data

REM Check if running as administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script must be run as Administrator
    echo Right-click on the script and select "Run as administrator"
    pause
    exit /b 1
)

REM Function to display usage
:usage
echo.
echo Simple NTP Daemon Windows Service Manager
echo =========================================
echo.
echo Usage: %0 [install^|uninstall^|start^|stop^|restart^|status^|config]
echo.
echo Commands:
echo   install   - Install the NTP service
echo   uninstall - Remove the NTP service
echo   start     - Start the NTP service
echo   stop      - Stop the NTP service
echo   restart   - Restart the NTP service
echo   status    - Show service status
echo   config    - Create configuration directory and files
echo.
if "%1"=="" (
    set /p choice="Enter command: "
    set "1=!choice!"
)

REM Main command processing
if "%1"=="install" goto install
if "%1"=="uninstall" goto uninstall
if "%1"=="start" goto start
if "%1"=="stop" goto stop
if "%1"=="restart" goto restart
if "%1"=="status" goto status
if "%1"=="config" goto config
goto usage

REM Install the service
:install
echo Installing Simple NTP Daemon service...
echo.

REM Check if binary exists
if not exist "%BINARY_PATH%" (
    echo ERROR: Service binary not found at %BINARY_PATH%
    echo Please build the service binary first
    pause
    exit /b 1
)

REM Check if service already exists
sc query "%SERVICE_NAME%" >nul 2>&1
if %errorLevel% equ 0 (
    echo Service %SERVICE_NAME% already exists
    set /p choice="Do you want to reinstall it? (y/N): "
    if /i "!choice!"=="y" (
        echo Uninstalling existing service...
        call :uninstall
    ) else (
        echo Installation cancelled
        pause
        exit /b 0
    )
)

REM Create service
echo Creating service %SERVICE_NAME%...
sc create "%SERVICE_NAME%" binPath= "%BINARY_PATH%" DisplayName= "%SERVICE_DISPLAY_NAME%" start= auto

if %errorLevel% neq 0 (
    echo ERROR: Failed to create service
    pause
    exit /b 1
)

REM Set service description
sc description "%SERVICE_NAME%" "%SERVICE_DESCRIPTION%"

REM Set service dependencies
sc config "%SERVICE_NAME%" depend= Tcpip

REM Set service failure actions
sc failure "%SERVICE_NAME%" reset= 86400 actions= restart/30000/restart/60000/restart/120000

echo.
echo Service installed successfully!
echo.
echo Next steps:
echo 1. Run '%0 config' to create configuration files
echo 2. Run '%0 start' to start the service
echo 3. Run '%0 status' to check service status
echo.
pause
exit /b 0

REM Uninstall the service
:uninstall
echo Uninstalling Simple NTP Daemon service...
echo.

REM Stop service if running
sc query "%SERVICE_NAME%" >nul 2>&1
if %errorLevel% equ 0 (
    echo Stopping service...
    sc stop "%SERVICE_NAME%" >nul 2>&1
    
    REM Wait for service to stop
    :wait_stop
    sc query "%SERVICE_NAME%" | find "STOPPED" >nul 2>&1
    if %errorLevel% neq 0 (
        timeout /t 1 /nobreak >nul
        goto wait_stop
    )
)

REM Delete service
echo Removing service...
sc delete "%SERVICE_NAME%"

if %errorLevel% equ 0 (
    echo Service removed successfully!
) else (
    echo Service was not found or could not be removed
)

echo.
pause
exit /b 0

REM Start the service
:start
echo Starting Simple NTP Daemon service...
sc start "%SERVICE_NAME%"

if %errorLevel% equ 0 (
    echo Service started successfully!
) else (
    echo ERROR: Failed to start service
    echo Check the Windows Event Log for details
)

echo.
pause
exit /b 0

REM Stop the service
:stop
echo Stopping Simple NTP Daemon service...
sc stop "%SERVICE_NAME%"

if %errorLevel% equ 0 (
    echo Service stopped successfully!
) else (
    echo ERROR: Failed to stop service
)

echo.
pause
exit /b 0

REM Restart the service
:restart
echo Restarting Simple NTP Daemon service...
call :stop
timeout /t 2 /nobreak >nul
call :start
exit /b 0

REM Show service status
:status
echo Simple NTP Daemon Service Status
echo ================================
echo.
sc query "%SERVICE_NAME%"
echo.
pause
exit /b 0

REM Create configuration
:config
echo Creating Simple NTP Daemon configuration...
echo.

REM Create directories
echo Creating directories...
if not exist "%CONFIG_DIR%" mkdir "%CONFIG_DIR%"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
if not exist "%DATA_DIR%" mkdir "%DATA_DIR%"

REM Create configuration file
echo Creating configuration file...
(
echo # Simple NTP Daemon Configuration File
echo # Generated on %date% %time%
echo.
echo # Network configuration
echo listen_address = 0.0.0.0
echo listen_port = 123
echo enable_ipv6 = true
echo max_connections = 1000
echo.
echo # NTP server configuration
echo stratum = 2
echo reference_clock = LOCAL
echo upstream_servers = pool.ntp.org,time.nist.gov
echo sync_interval = 64
echo timeout = 1000
echo.
echo # Logging configuration
echo log_level = 1
echo log_file = %LOG_DIR%\simple-ntpd.log
echo enable_console_logging = false
echo enable_syslog = false
echo.
echo # Security configuration
echo enable_authentication = false
echo authentication_key = 
echo restrict_queries = false
echo allowed_clients = 0.0.0.0/0
echo denied_clients = 
echo.
echo # Performance configuration
echo worker_threads = 4
echo max_packet_size = 1024
echo enable_statistics = true
echo stats_interval = 60
echo.
echo # Drift file configuration
echo drift_file = %DATA_DIR%\drift
echo enable_drift_compensation = true
echo.
echo # Leap second configuration
echo leap_second_file = %DATA_DIR%\leap-seconds.list
echo enable_leap_second_handling = true
) > "%CONFIG_DIR%\simple-ntpd.conf"

REM Create log files
echo Creating log files...
echo. > "%LOG_DIR%\simple-ntpd.log"
echo. > "%LOG_DIR%\simple-ntpd.error.log"

REM Set permissions (basic - in production you'd want more security)
echo Setting permissions...
icacls "%CONFIG_DIR%" /grant "NT AUTHORITY\SYSTEM:(OI)(CI)F" /T
icacls "%LOG_DIR%" /grant "NT AUTHORITY\SYSTEM:(OI)(CI)F" /T
icacls "%DATA_DIR%" /grant "NT AUTHORITY\SYSTEM:(OI)(CI)F" /T

echo.
echo Configuration created successfully!
echo.
echo Configuration file: %CONFIG_DIR%\simple-ntpd.conf
echo Log directory: %LOG_DIR%
echo Data directory: %DATA_DIR%
echo.
echo You can now edit the configuration file and start the service.
echo.
pause
exit /b 0
