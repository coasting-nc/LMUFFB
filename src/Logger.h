#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdarg>

// Simple synchronous logger that flushes every line for crash debugging
class Logger {
public:
    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    void Init(const std::string& filename) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file.close();
        }
        m_filename = filename;
        m_file.open(m_filename, std::ios::out | std::ios::trunc);
        if (m_file.is_open()) {
            m_initialized = true;
            _LogNoLock("Logger Initialized. Version: " + std::string(LMUFFB_VERSION), true);
        }
    }

    void Log(const char* fmt, ...) {
        char buffer[2048];
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        std::string message(buffer);

        std::lock_guard<std::mutex> lock(m_mutex);
        _LogNoLock(message, true);
    }

    // Log to file only, not to console
    void LogFile(const char* fmt, ...) {
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        std::string message(buffer);

        std::lock_guard<std::mutex> lock(m_mutex);
        _LogNoLock(message, false);
    }

    // Helper for std::string
    void LogStr(const std::string& msg) {
        Log("%s", msg.c_str());
    }

    // Helper for std::string (file only)
    void LogFileStr(const std::string& msg) {
        LogFile("%s", msg.c_str());
    }

    // Helper for error logging with GetLastError()
    void LogWin32Error(const char* context, unsigned long errorCode) {
        Log("Error in %s: Code %lu", context, errorCode);
    }

    const std::string& GetFilename() const { return m_filename; }

    // For testing
    void SetTestStream(std::ostream* os) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_testStream = os;
    }

private:
    Logger() {}
    ~Logger() noexcept {
        try {
            if (m_file.is_open()) {
                m_file << "Logger Shutdown.\n";
                m_file.close();
            }
        } catch (...) {
            // Destructor must not throw
        }
    }

    void _LogNoLock(const std::string& message, bool toConsole) {
        if (m_testStream) {
            *m_testStream << message << "\n";
        }

        if (m_file.is_open()) {
            // Timestamp
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm time_info;
            #ifdef _WIN32
                localtime_s(&time_info, &in_time_t);
            #else
                localtime_r(&in_time_t, &time_info);
            #endif

            m_file << "[" << std::put_time(&time_info, "%H:%M:%S") << "] " << message << "\n";
            m_file.flush(); // Critical for crash debugging
        }
            m_file << "[" << std::put_time(&time_info, "%H:%M:%S") << "] " << message << "\n";
            m_file.flush(); // Critical for crash debugging
        }

        if (toConsole) {
            // Also print to console for consistency
            // Re-calculate timestamp if not already done (file logging also calculates it)
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm time_info;
            #ifdef _WIN32
                localtime_s(&time_info, &in_time_t);
            #else
                localtime_r(&in_time_t, &time_info);
            #endif

            std::cout << "[" << std::put_time(&time_info, "%H:%M:%S") << "] [Log] " << message << std::endl;
        }
    }

    std::string m_filename;
    std::ofstream m_file;
    std::mutex m_mutex;
    bool m_initialized = false;
    std::ostream* m_testStream = nullptr;
};

#endif // LOGGER_H
