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

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <signal.h>
#include <csignal>
#include "simple_ntpd/ntp_server.hpp"
#include "simple_ntpd/ntp_config.hpp"
#include "simple_ntpd/logger.hpp"

using namespace simple_ntpd;

// Global variables for signal handling
std::shared_ptr<NtpServer> g_server;
Logger* g_logger = nullptr;
std::atomic<bool> g_shutdown_requested(false);

/**
 * @brief Signal handler for graceful shutdown
 * @param signal Signal number
 */
void signalHandler(int signal) {
    if (g_shutdown_requested.exchange(true)) {
        // Already shutting down, force exit
        std::exit(1);
    }
    
    g_logger->info("Received signal " + std::to_string(signal) + ", initiating graceful shutdown");
    
    if (g_server) {
        g_server->stop();
    }
}

/**
 * @brief Print usage information
 */
void printUsage() {
    std::cout << "\nUsage: simple-ntpd [OPTIONS] [COMMAND] [ARGS...]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --help, -h           Show this help message" << std::endl;
    std::cout << "  --version, -v        Show version information" << std::endl;
    std::cout << "  --config, -c FILE    Use specified configuration file" << std::endl;
    std::cout << "  --verbose, -V        Enable verbose logging" << std::endl;
    std::cout << "  --daemon, -d         Run as daemon" << std::endl;
    std::cout << "  --foreground, -f     Run in foreground" << std::endl;
    std::cout << "  --test-config        Test configuration file" << std::endl;
    std::cout << "  --validate           Validate configuration" << std::endl;
    std::cout << "  --listen ADDR        Listen on specific address" << std::endl;
    std::cout << "  --port PORT          Listen on specific port" << std::endl;
    std::cout << "  --stratum LEVEL      Set stratum level (1-15)" << std::endl;
    std::cout << "  --reference-id ID    Set reference identifier" << std::endl;
    
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  start                Start the NTP server" << std::endl;
    std::cout << "  stop                 Stop the NTP server" << std::endl;
    std::cout << "  restart              Restart the NTP server" << std::endl;
    std::cout << "  status               Show server status" << std::endl;
    std::cout << "  reload               Reload configuration" << std::endl;
    std::cout << "  test                 Test server configuration" << std::endl;
    std::cout << "  stats                Show server statistics" << std::endl;
    std::cout << "  connections          List active connections" << std::endl;
    
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  simple-ntpd start --config /etc/simple-ntpd/config.conf" << std::endl;
    std::cout << "  simple-ntpd start --listen 0.0.0.0 --port 123 --stratum 2" << std::endl;
    std::cout << "  simple-ntpd --daemon start" << std::endl;
    std::cout << "  simple-ntpd status" << std::endl;
}

/**
 * @brief Print version information
 */
void printVersion() {
    std::cout << "simple-ntpd v0.1.0" << std::endl;
    std::cout << "Simple NTP Daemon for Linux, macOS, and Windows" << std::endl;
    std::cout << "Copyright (c) 2024 BLBurns <contact@blburns.com>" << std::endl;
    std::cout << "Licensed under Apache License 2.0" << std::endl;
}

/**
 * @brief Parse command line arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @param config Configuration object to populate
 * @return true if parsed successfully, false otherwise
 */
bool parseCommandLine(int argc, char* argv[], std::shared_ptr<NtpConfig>& config) {
    std::string command;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        } else if (arg == "--version" || arg == "-v") {
            printVersion();
            return false;
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                // Load from config file
                if (!config->loadFromFile(argv[++i])) {
                    std::cerr << "Error: Failed to load configuration file: " << argv[i] << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Error: --config requires a file path" << std::endl;
                return false;
            }
        } else if (arg == "--verbose" || arg == "-V") {
            config->log_level = LogLevel::DEBUG;
        } else if (arg == "--daemon" || arg == "-d") {
            config->enable_console_logging = false;
        } else if (arg == "--foreground" || arg == "-f") {
            config->enable_console_logging = true;
        } else if (arg == "--listen") {
            if (i + 1 < argc) {
                config->listen_address = argv[++i];
            } else {
                std::cerr << "Error: --listen requires an address" << std::endl;
                return false;
            }
        } else if (arg == "--port") {
            if (i + 1 < argc) {
                try {
                    config->listen_port = static_cast<port_t>(std::stoi(argv[++i]));
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid port number: " << argv[i] << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Error: --port requires a port number" << std::endl;
                return false;
            }
        } else if (arg == "--stratum") {
            if (i + 1 < argc) {
                try {
                    int stratum = std::stoi(argv[++i]);
                    if (stratum >= 1 && stratum <= 15) {
                        config->stratum = static_cast<NtpStratum>(stratum);
                    } else {
                        std::cerr << "Error: Stratum must be between 1 and 15" << std::endl;
                        return false;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid stratum level: " << argv[i] << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Error: --stratum requires a level" << std::endl;
                return false;
            }
        } else if (arg == "--reference-id") {
            if (i + 1 < argc) {
                config->reference_id = argv[++i];
            } else {
                std::cerr << "Error: --reference-id requires an identifier" << std::endl;
                return false;
            }
        } else if (arg == "--test-config") {
            if (config->validate()) {
                std::cout << "Configuration is valid" << std::endl;
            } else {
                std::cerr << "Configuration validation failed" << std::endl;
                return false;
            }
            return false;
        } else if (arg == "--validate") {
            if (config->validate()) {
                std::cout << "Configuration is valid" << std::endl;
            } else {
                std::cerr << "Configuration validation failed" << std::endl;
                return false;
            }
            return false;
        } else if (arg[0] != '-') {
            // This is a command
            command = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            printUsage();
            return false;
        }
    }
    
    // If no command specified, default to start
    if (command.empty()) {
        command = "start";
    }
    
    // Handle commands
    if (command == "start") {
        // Start server logic will be handled in main
        return true;
    } else if (command == "stop") {
        std::cout << "Stop command not implemented yet" << std::endl;
        return false;
    } else if (command == "restart") {
        std::cout << "Restart command not implemented yet" << std::endl;
        return false;
    } else if (command == "status") {
        std::cout << "Status command not implemented yet" << std::endl;
        return false;
    } else if (command == "reload") {
        std::cout << "Reload command not implemented yet" << std::endl;
        return false;
    } else if (command == "test") {
        std::cout << "Test command not implemented yet" << std::endl;
        return false;
    } else if (command == "stats") {
        std::cout << "Stats command not implemented yet" << std::endl;
        return false;
    } else if (command == "connections") {
        std::cout << "Connections command not implemented yet" << std::endl;
        return false;
    } else {
        std::cerr << "Error: Unknown command: " << command << std::endl;
        printUsage();
        return false;
    }
    
    return true;
}

/**
 * @brief Initialize signal handlers
 */
void initializeSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    
    #ifdef _WIN32
    signal(SIGBREAK, signalHandler);
    #else
    signal(SIGHUP, signalHandler);
    #endif
}

/**
 * @brief Main function
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    try {
        // Initialize configuration
        auto config = std::make_shared<NtpConfig>();
        
        // Parse command line arguments
        if (!parseCommandLine(argc, argv, config)) {
            return 0; // Help or version printed
        }
        
        // Initialize logger
        g_logger = &Logger::getInstance();
        
        // Configure logger
        g_logger->setLogFile(config->log_file);
        g_logger->setLevel(config->log_level);
        if (config->enable_console_logging) {
            g_logger->setDestination(LogDestination::CONSOLE);
        } else {
            g_logger->setDestination(LogDestination::FILE);
        }
        
        // Log startup
        g_logger->info("Starting simple-ntpd v0.1.0");
        g_logger->info("Configuration: " + config->toString());
        
        // Initialize signal handlers
        initializeSignalHandlers();
        
        // Create and start server
        g_server = std::make_shared<NtpServer>(config, std::shared_ptr<Logger>(&Logger::getInstance(), [](Logger*){}));
        
        if (!g_server->start()) {
            g_logger->error("Failed to start NTP server");
            return 1;
        }
        
        g_logger->info("NTP server started successfully");
        g_logger->info("Listening on " + config->listen_address + ":" + std::to_string(config->listen_port));
        
        // Main server loop
        while (g_server->isRunning() && !g_shutdown_requested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Shutdown
        g_logger->info("Shutting down NTP server");
        g_server->stop();
        
        g_logger->info("NTP server shutdown complete");
        
    } catch (const std::exception& e) {
        if (g_logger) {
            g_logger->fatal("Fatal error: " + std::string(e.what()));
        } else {
            std::cerr << "Fatal error: " << e.what() << std::endl;
        }
        return 1;
    } catch (...) {
        if (g_logger) {
            g_logger->fatal("Unknown fatal error occurred");
        } else {
            std::cerr << "Unknown fatal error occurred" << std::endl;
        }
        return 1;
    }
    
    return 0;
}
