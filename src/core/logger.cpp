#include "simple_ntpd/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <syslog.h>
#endif

namespace simple_ntpd {

namespace {
    // Convert log level to string
    const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::FATAL:   return "FATAL";
            default:                return "UNKNOWN";
        }
    }

    // Convert log level to syslog priority
    int levelToSyslogPriority(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return LOG_DEBUG;
            case LogLevel::INFO:    return LOG_INFO;
            case LogLevel::WARNING: return LOG_WARNING;
            case LogLevel::ERROR:   return LOG_ERR;
            case LogLevel::FATAL:   return LOG_CRIT;
            default:                return LOG_INFO;
        }
    }

    // Get current timestamp string
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    // Get thread ID string
    std::string getThreadId() {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    }
}

// Logger implementation
class Logger::Impl {
public:
    Impl() : level_(LogLevel::INFO), 
             destination_(LogDestination::CONSOLE),
             log_file_(),
             enable_syslog_(false),
             syslog_facility_(LOG_DAEMON),
             mutex_() {
        
        // Initialize syslog on Unix-like systems
#ifndef _WIN32
        if (enable_syslog_) {
            openlog("simple-ntpd", LOG_PID | LOG_CONS, syslog_facility_);
        }
#endif
    }

    ~Impl() {
#ifndef _WIN32
        if (enable_syslog_) {
            closelog();
        }
#endif
    }

    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    void setDestination(LogDestination destination) {
        std::lock_guard<std::mutex> lock(mutex_);
        destination_ = destination;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        log_file_ = filename;
    }

    void setSyslog(bool enable, int facility) {
        std::lock_guard<std::mutex> lock(mutex_);
        enable_syslog_ = enable;
        syslog_facility_ = facility;
        
#ifndef _WIN32
        if (enable_syslog_) {
            openlog("simple-ntpd", LOG_PID | LOG_CONS, syslog_facility_);
        } else {
            closelog();
        }
#endif
    }

    void log(LogLevel level, const std::string& message, const std::string& file, int line) {
        if (level < level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        // Format the log message
        std::string formatted_message = formatMessage(level, message, file, line);
        
        // Output to console
        if (destination_ == LogDestination::CONSOLE || destination_ == LogDestination::BOTH) {
            outputToConsole(level, formatted_message);
        }
        
        // Output to file
        if ((destination_ == LogDestination::FILE || destination_ == LogDestination::BOTH) && !log_file_.empty()) {
            outputToFile(formatted_message);
        }
        
        // Output to syslog
        if (enable_syslog_) {
            outputToSyslog(level, message);
        }
    }

private:
    std::string formatMessage(LogLevel level, const std::string& message, 
                             const std::string& file, int line) {
        std::stringstream ss;
        ss << "[" << getCurrentTimestamp() << "] "
           << "[" << levelToString(level) << "] "
           << "[" << getThreadId() << "] ";
        
        if (!file.empty()) {
            ss << "[" << file << ":" << line << "] ";
        }
        
        ss << message;
        return ss.str();
    }

    void outputToConsole(LogLevel level, const std::string& message) {
        // Set console color based on log level (Windows 10+ and Unix-like systems)
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color;
        switch (level) {
            case LogLevel::DEBUG:   color = FOREGROUND_INTENSITY; break;
            case LogLevel::INFO:    color = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case LogLevel::WARNING: color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case LogLevel::ERROR:   color = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case LogLevel::FATAL:   color = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            default:                color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        }
        SetConsoleTextAttribute(hConsole, color);
        std::cout << message << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
        // ANSI color codes for Unix-like systems
        const char* color_code = "";
        switch (level) {
            case LogLevel::DEBUG:   color_code = "\033[36m"; break;  // Cyan
            case LogLevel::INFO:    color_code = "\033[32m"; break;  // Green
            case LogLevel::WARNING: color_code = "\033[33m"; break;  // Yellow
            case LogLevel::ERROR:   color_code = "\033[31m"; break;  // Red
            case LogLevel::FATAL:   color_code = "\033[35m"; break;  // Magenta
            default:                color_code = "\033[0m"; break;   // Reset
        }
        std::cout << color_code << message << "\033[0m" << std::endl;
#endif
    }

    void outputToFile(const std::string& message) {
        std::ofstream file(log_file_, std::ios::app);
        if (file.is_open()) {
            file << message << std::endl;
            file.close();
        }
    }

    void outputToSyslog(LogLevel level, const std::string& message) {
#ifndef _WIN32
        int priority = levelToSyslogPriority(level);
        syslog(priority, "%s", message.c_str());
#endif
    }

    LogLevel level_;
    LogDestination destination_;
    std::string log_file_;
    bool enable_syslog_;
    int syslog_facility_;
    mutable std::mutex mutex_;
};

// Logger public interface implementation
Logger::Logger() : impl_(std::make_unique<Impl>()) {}

Logger::~Logger() = default;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) {
    impl_->setLevel(level);
}

void Logger::setDestination(LogDestination destination) {
    impl_->setDestination(destination);
}

void Logger::setLogFile(const std::string& filename) {
    impl_->setLogFile(filename);
}

void Logger::setSyslog(bool enable, int facility) {
    impl_->setSyslog(enable, facility);
}

void Logger::log(LogLevel level, const std::string& message, const std::string& file, int line) {
    impl_->log(level, message, file, line);
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line) {
    log(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::fatal(const std::string& message, const std::string& file, int line) {
    log(LogLevel::FATAL, message, file, line);
}

} // namespace simple_ntpd
