/**
 * @file windows_service.hpp
 * @brief Windows Service Control Manager entry point
 * @license Apache-2.0
 */

#pragma once

#include <iostream>
#include <string>

namespace simple_ntpd {

/**
 * Handles SCM startup and `service install|status|uninstall`.
 * Returns true when this process should exit with exit_code.
 * Returns false when the process was started as a normal console program.
 */
#ifdef _WIN32
bool windows_service_entry(int argc, char** argv, int& exit_code,
                           void (*request_shutdown)(),
                           int (*run)(int, char**));
#else
inline bool windows_service_entry(int argc, char** argv, int& exit_code,
                                  void (*)(), int (*)(int, char**)) {
    if (argc > 1 && std::string(argv[1]) == "service") {
        std::cerr << "Windows service commands are only available on Windows\n";
        exit_code = 1;
        return true;
    }
    return false;
}
#endif

}  // namespace simple_ntpd
