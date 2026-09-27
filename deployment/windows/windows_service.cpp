/**
 * @file windows_service.cpp
 * @brief Register and run simple-ntpd with the Service Control Manager
 * @license Apache-2.0
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "simple-ntpd/platform/windows_service.hpp"

#include <windows.h>
#include <winsvc.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace simple_ntpd {
namespace {

constexpr char kServiceName[] = "simple-ntpd";
constexpr char kDisplayName[] = "Simple NTP Daemon";
constexpr char kDescription[] = "Provides NTP time synchronization";

SERVICE_STATUS g_status{};
SERVICE_STATUS_HANDLE g_status_handle = nullptr;
HANDLE g_stop_event = nullptr;
void (*g_request_shutdown)() = nullptr;
int (*g_run)(int, char**) = nullptr;
int g_argc = 0;
char** g_argv = nullptr;

void report_status(DWORD state, DWORD exit_code, DWORD wait_hint) {
    static DWORD checkpoint = 1;
    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwCurrentState = state;
    g_status.dwWin32ExitCode = exit_code;
    g_status.dwWaitHint = wait_hint;
    g_status.dwControlsAccepted =
        (state == SERVICE_START_PENDING) ? 0 : SERVICE_ACCEPT_STOP;
    if (state == SERVICE_RUNNING || state == SERVICE_STOPPED) {
        g_status.dwCheckPoint = 0;
    } else {
        g_status.dwCheckPoint = checkpoint++;
    }
    if (g_status_handle != nullptr) {
        SetServiceStatus(g_status_handle, &g_status);
    }
}

void WINAPI service_ctrl(DWORD control) {
    if (control == SERVICE_CONTROL_STOP) {
        report_status(SERVICE_STOP_PENDING, NO_ERROR, 3000);
        if (g_request_shutdown != nullptr) {
            g_request_shutdown();
        }
        if (g_stop_event != nullptr) {
            SetEvent(g_stop_event);
        }
        return;
    }
    report_status(g_status.dwCurrentState, NO_ERROR, 0);
}

void WINAPI service_main(DWORD, LPSTR*) {
    g_status_handle = RegisterServiceCtrlHandlerA(kServiceName, service_ctrl);
    if (g_status_handle == nullptr) {
        return;
    }
    report_status(SERVICE_START_PENDING, NO_ERROR, 3000);
    g_stop_event = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (g_stop_event == nullptr) {
        report_status(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    std::thread worker([] {
        if (g_run != nullptr) {
            g_run(g_argc, g_argv);
        }
        if (g_stop_event != nullptr) {
            SetEvent(g_stop_event);
        }
    });

    report_status(SERVICE_RUNNING, NO_ERROR, 0);
    WaitForSingleObject(g_stop_event, INFINITE);
    if (g_request_shutdown != nullptr) {
        g_request_shutdown();
    }
    if (worker.joinable()) {
        worker.join();
    }
    CloseHandle(g_stop_event);
    g_stop_event = nullptr;
    report_status(SERVICE_STOPPED, NO_ERROR, 0);
}

std::string module_path() {
    char path[MAX_PATH];
    const DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return {};
    }
    std::string quoted = "\"";
    quoted += path;
    quoted += "\"";
    return quoted;
}

const char* state_name(DWORD state) {
    switch (state) {
    case SERVICE_STOPPED:
        return "STOPPED";
    case SERVICE_START_PENDING:
        return "START_PENDING";
    case SERVICE_STOP_PENDING:
        return "STOP_PENDING";
    case SERVICE_RUNNING:
        return "RUNNING";
    case SERVICE_CONTINUE_PENDING:
        return "CONTINUE_PENDING";
    case SERVICE_PAUSE_PENDING:
        return "PAUSE_PENDING";
    case SERVICE_PAUSED:
        return "PAUSED";
    default:
        return "UNKNOWN";
    }
}

const char* start_name(DWORD start) {
    switch (start) {
    case SERVICE_AUTO_START:
        return "AUTO_START";
    case SERVICE_DEMAND_START:
        return "DEMAND_START";
    case SERVICE_DISABLED:
        return "DISABLED";
    case SERVICE_BOOT_START:
        return "BOOT_START";
    case SERVICE_SYSTEM_START:
        return "SYSTEM_START";
    default:
        return "UNKNOWN";
    }
}

int install_service() {
    const std::string binary = module_path();
    if (binary.empty()) {
        std::fprintf(stderr, "Cannot locate simple-ntpd.exe (%lu)\n", GetLastError());
        return 1;
    }
    SC_HANDLE manager = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (manager == nullptr) {
        std::fprintf(stderr, "OpenSCManager failed (%lu). Run from an elevated prompt.\n",
                     GetLastError());
        return 1;
    }
    SC_HANDLE service = CreateServiceA(
        manager, kServiceName, kDisplayName, SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        binary.c_str(), nullptr, nullptr, "Tcpip\0", nullptr, nullptr);
    if (service == nullptr) {
        const DWORD err = GetLastError();
        CloseServiceHandle(manager);
        if (err == ERROR_SERVICE_EXISTS) {
            std::printf("Service %s is already registered\n", kServiceName);
            return 0;
        }
        std::fprintf(stderr, "CreateService failed (%lu)\n", err);
        return 1;
    }
    SERVICE_DESCRIPTIONA description;
    description.lpDescription = const_cast<char*>(kDescription);
    ChangeServiceConfig2A(service, SERVICE_CONFIG_DESCRIPTION, &description);
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    std::printf("Registered %s (%s)\n", kServiceName, kDisplayName);
    std::printf("It is visible in services.msc. Start type is Automatic.\n");
    return 0;
}

int uninstall_service() {
    SC_HANDLE manager = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        std::fprintf(stderr, "OpenSCManager failed (%lu)\n", GetLastError());
        return 1;
    }
    SC_HANDLE service = OpenServiceA(manager, kServiceName, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (service == nullptr) {
        const DWORD err = GetLastError();
        CloseServiceHandle(manager);
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            std::printf("Service %s is not registered\n", kServiceName);
            return 0;
        }
        std::fprintf(stderr, "OpenService failed (%lu)\n", err);
        return 1;
    }
    SERVICE_STATUS status{};
    ControlService(service, SERVICE_CONTROL_STOP, &status);
    const BOOL deleted = DeleteService(service);
    const DWORD err = GetLastError();
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    if (!deleted && err != ERROR_SERVICE_MARKED_FOR_DELETE) {
        std::fprintf(stderr, "DeleteService failed (%lu)\n", err);
        return 1;
    }
    std::printf("Removed service %s\n", kServiceName);
    return 0;
}

int status_service() {
    SC_HANDLE manager = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        std::fprintf(stderr, "OpenSCManager failed (%lu)\n", GetLastError());
        return 1;
    }
    SC_HANDLE service = OpenServiceA(manager, kServiceName, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if (service == nullptr) {
        const DWORD err = GetLastError();
        CloseServiceHandle(manager);
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            std::printf("Service %s is not registered\n", kServiceName);
            return 1;
        }
        std::fprintf(stderr, "OpenService failed (%lu)\n", err);
        return 1;
    }
    SERVICE_STATUS_PROCESS status{};
    DWORD needed = 0;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                              reinterpret_cast<LPBYTE>(&status), sizeof(status),
                              &needed)) {
        std::fprintf(stderr, "QueryServiceStatusEx failed (%lu)\n", GetLastError());
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return 1;
    }
    DWORD bytes = 0;
    QueryServiceConfigA(service, nullptr, 0, &bytes);
    std::vector<unsigned char> config_buf(bytes == 0 ? 1 : bytes);
    auto* config = reinterpret_cast<QUERY_SERVICE_CONFIGA*>(config_buf.data());
    const BOOL have_config =
        bytes > 0 && QueryServiceConfigA(service, config, bytes, &bytes);
    std::printf("Service: %s\n", kServiceName);
    std::printf("Display name: %s\n",
                have_config && config->lpDisplayName ? config->lpDisplayName : kDisplayName);
    std::printf("State: %s\n", state_name(status.dwCurrentState));
    if (have_config) {
        std::printf("Start type: %s\n", start_name(config->dwStartType));
        std::printf("Binary: %s\n", config->lpBinaryPathName ? config->lpBinaryPathName : "");
    }
    std::printf("Registered: yes\n");
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return 0;
}

int handle_service_command(int argc, char** argv) {
    const char* action = argc > 2 ? argv[2] : "status";
    if (std::strcmp(action, "install") == 0) {
        return install_service();
    }
    if (std::strcmp(action, "uninstall") == 0) {
        return uninstall_service();
    }
    if (std::strcmp(action, "status") == 0) {
        return status_service();
    }
    std::fprintf(stderr, "Usage: simple-ntpd service [install|status|uninstall]\n");
    return 1;
}

}  // namespace

bool windows_service_entry(int argc, char** argv, int& exit_code,
                           void (*request_shutdown)(),
                           int (*run)(int, char**)) {
    if (argc > 1 && std::strcmp(argv[1], "service") == 0) {
        exit_code = handle_service_command(argc, argv);
        return true;
    }

    g_request_shutdown = request_shutdown;
    g_run = run;
    g_argc = argc;
    g_argv = argv;

    SERVICE_TABLE_ENTRYA table[] = {
        {const_cast<LPSTR>(kServiceName), service_main},
        {nullptr, nullptr},
    };
    if (StartServiceCtrlDispatcherA(table)) {
        exit_code = 0;
        return true;
    }
    if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
        return false;
    }
    std::fprintf(stderr, "StartServiceCtrlDispatcher failed (%lu)\n", GetLastError());
    exit_code = 1;
    return true;
}

}  // namespace simple_ntpd
