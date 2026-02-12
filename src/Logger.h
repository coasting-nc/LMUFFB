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
        m_filename = filename;
        m_file.open(m_filename, std::ios::out | std::ios::trunc);
        if (m_file.is_open()) {
            m_initialized = true;
            _LogNoLock("Logger Initialized. Version: " + std::string(LMUFFB_VERSION));
        }
    }

    void Log(const char* fmt, ...) {
        if (!m_initialized) return;

        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        std::string message(buffer);

        std::lock_guard<std::mutex> lock(m_mutex);
        _LogNoLock(message);
    }

    // Helper for std::string
    void LogStr(const std::string& msg) {
        Log("%s", msg.c_str());
    }

    // Helper for error logging with GetLastError()
    void LogWin32Error(const char* context, unsigned long errorCode) {
        Log("Error in %s: Code %lu", context, errorCode);
    }

private:
    Logger() {}
    ~Logger() {
        if (m_file.is_open()) {
            m_file << "Logger Shutdown.\n";
            m_file.close();
        }
    }

    void _LogNoLock(const std::string& message) {
        if (!m_file.is_open()) return;

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

        // Also print to console for consistency
        std::cout << "[Log] " << message << std::endl;
    }

    std::string m_filename;
    std::ofstream m_file;
    std::mutex m_mutex;
    bool m_initialized = false;
};

#endif // LOGGER_H
