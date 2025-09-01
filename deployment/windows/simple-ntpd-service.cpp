/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

// Service configuration
#define SERVICE_NAME _T("SimpleNtpd")
#define SERVICE_DISPLAY_NAME _T("Simple NTP Daemon")
#define SERVICE_DESCRIPTION _T("Provides NTP time synchronization services")
#define SERVICE_TYPE SERVICE_WIN32_OWN_PROCESS
#define SERVICE_START_TYPE SERVICE_AUTO_START
#define SERVICE_ERROR_CONTROL SERVICE_ERROR_NORMAL

// Global variables
SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;
std::unique_ptr<std::thread> g_ServiceThread;
bool g_ServiceRunning = false;

// Function prototypes
VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD dwCtrl);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
VOID InstallService();
VOID UninstallService();
VOID StartNtpDaemon();
VOID StopNtpDaemon();

// Service main function
VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv) {
    // Register the handler function for the service
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    
    if (g_StatusHandle == NULL) {
        return;
    }
    
    // These SERVICE_STATUS members remain as set here
    g_ServiceStatus.dwServiceType = SERVICE_TYPE;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    
    // Report initial status to the SCM
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    
    // Create an event. The control handler function, ServiceCtrlHandler,
    // signals this event when it receives the stop control code.
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL) {
        ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }
    
    // Report running status when initialization is complete
    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
    
    // Start the NTP daemon
    StartNtpDaemon();
    
    // Start the service worker thread
    g_ServiceThread = std::make_unique<std::thread>(ServiceWorkerThread, NULL);
    
    // Wait until the service is stopped
    WaitForSingleObject(g_ServiceStopEvent, INFINITE);
    
    // Stop the NTP daemon
    StopNtpDaemon();
    
    // Report stopped status
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

// Service control handler function
VOID WINAPI ServiceCtrlHandler(DWORD dwCtrl) {
    switch (dwCtrl) {
        case SERVICE_CONTROL_STOP:
            ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            
            // Signal the service to stop
            SetEvent(g_ServiceStopEvent);
            g_ServiceRunning = false;
            break;
            
        case SERVICE_CONTROL_INTERROGATE:
            // Just report the current status
            break;
            
        default:
            break;
    }
    
    ReportSvcStatus(g_ServiceStatus.dwCurrentState, NO_ERROR, 0);
}

// Service worker thread function
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    g_ServiceRunning = true;
    
    while (g_ServiceRunning) {
        // Check if the service is stopping
        if (WaitForSingleObject(g_ServiceStopEvent, 1000) == WAIT_OBJECT_0) {
            break;
        }
        
        // Service main loop - keep the service alive
        // The actual NTP daemon runs in a separate process
    }
    
    return ERROR_SUCCESS;
}

// Report service status to the SCM
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint) {
    static DWORD dwCheckPoint = 1;
    
    // Fill in the SERVICE_STATUS structure
    g_ServiceStatus.dwCurrentState = dwCurrentState;
    g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    g_ServiceStatus.dwWaitHint = dwWaitHint;
    
    if (dwCurrentState == SERVICE_START_PENDING) {
        g_ServiceStatus.dwControlsAccepted = 0;
    } else {
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }
    
    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) {
        g_ServiceStatus.dwCheckPoint = 0;
    } else {
        g_ServiceStatus.dwCheckPoint = dwCheckPoint++;
    }
    
    // Report the status of the service to the SCM
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

// Install the service
VOID InstallService() {
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    TCHAR szPath[MAX_PATH];
    
    if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
        _tprintf(_T("Cannot install service (%d)\n"), GetLastError());
        return;
    }
    
    // Get a handle to the SCM database
    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database
        SC_MANAGER_ALL_ACCESS);  // full access rights
    
    if (NULL == schSCManager) {
        _tprintf(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }
    
    // Create the service
    schService = CreateService(
        schSCManager,              // SCM database
        SERVICE_NAME,              // name of service
        SERVICE_DISPLAY_NAME,      // service name to display
        SERVICE_ALL_ACCESS,        // desired access
        SERVICE_TYPE,              // service type
        SERVICE_START_TYPE,        // start type
        SERVICE_ERROR_CONTROL,     // error control type
        szPath,                    // path to service's binary
        NULL,                      // no load ordering group
        NULL,                      // no tag identifier
        NULL,                      // no dependencies
        NULL,                      // LocalSystem account
        NULL);                     // no password
    
    if (schService == NULL) {
        _tprintf(_T("CreateService failed (%d)\n"), GetLastError());
        CloseServiceHandle(schSCManager);
        return;
    }
    
    // Set service description
    SERVICE_DESCRIPTION sd;
    sd.lpDescription = SERVICE_DESCRIPTION;
    ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd);
    
    _tprintf(_T("Service installed successfully\n"));
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

// Uninstall the service
VOID UninstallService() {
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};
    
    // Get a handle to the SCM database
    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database
        SC_MANAGER_ALL_ACCESS);  // full access rights
    
    if (NULL == schSCManager) {
        _tprintf(_T("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }
    
    // Get a handle to the service
    schService = OpenService(
        schSCManager,       // SCM database
        SERVICE_NAME,       // name of service
        DELETE);            // need delete access
    
    if (schService == NULL) {
        _tprintf(_T("OpenService failed (%d)\n"), GetLastError());
        CloseServiceHandle(schSCManager);
        return;
    }
    
    // Stop the service if it's running
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus)) {
        _tprintf(_T("Stopping %s."), SERVICE_NAME);
        Sleep(1000);
        
        while (QueryServiceStatus(schService, &ssSvcStatus)) {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                _tprintf(_T("."));
                Sleep(1000);
            } else {
                break;
            }
        }
        
        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED) {
            _tprintf(_T("\n%s stopped.\n"), SERVICE_NAME);
        } else {
            _tprintf(_T("\n%s failed to stop.\n"), SERVICE_NAME);
        }
    }
    
    // Now remove the service
    if (!DeleteService(schService)) {
        _tprintf(_T("DeleteService failed (%d)\n"), GetLastError());
    } else {
        _tprintf(_T("Service removed successfully\n"));
    }
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

// Start the NTP daemon
VOID StartNtpDaemon() {
    // This would start the actual NTP daemon process
    // For now, just log that we're starting
    OutputDebugString(_T("Starting NTP daemon...\n"));
}

// Stop the NTP daemon
VOID StopNtpDaemon() {
    // This would stop the actual NTP daemon process
    // For now, just log that we're stopping
    OutputDebugString(_T("Stopping NTP daemon...\n"));
}

// Main function
int _tmain(int argc, TCHAR *argv[]) {
    // If command-line arguments are provided, handle them
    if (argc > 1) {
        if (lstrcmpi(argv[1], _T("install")) == 0) {
            InstallService();
            return 0;
        } else if (lstrcmpi(argv[1], _T("uninstall")) == 0) {
            UninstallService();
            return 0;
        } else if (lstrcmpi(argv[1], _T("run")) == 0) {
            // Run as a console application for testing
            _tprintf(_T("Running NTP daemon in console mode...\n"));
            StartNtpDaemon();
            
            _tprintf(_T("Press any key to stop...\n"));
            _getch();
            
            StopNtpDaemon();
            return 0;
        } else {
            _tprintf(_T("Usage: %s [install|uninstall|run]\n"), argv[0]);
            _tprintf(_T("  install   - Install the service\n"));
            _tprintf(_T("  uninstall - Uninstall the service\n"));
            _tprintf(_T("  run       - Run in console mode\n"));
            return 1;
        }
    }
    
    // If no arguments, run as a service
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };
    
    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
        return GetLastError();
    }
    
    return 0;
}
