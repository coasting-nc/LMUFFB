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
#include <ctime>
#include <cstdarg>
#include <algorithm> // For std::max, std::min
#include <filesystem>
#include "StringUtils.h" // Include StringUtils.h

// Simple synchronous logger that flushes every line for crash debugging
class Logger {
public:
    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    /**
     * Initialize the logger.
     * @param base_filename The base name of the log file (e.g. "debug.log")
     * @param log_path Optional directory to store the log file.
     * @param use_timestamp If true (default), appends a unique timestamp to the filename to avoid overwriting.
     */
    void Init(const std::string& base_filename, const std::string& log_path = "", bool use_timestamp = true) {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string final_filename = base_filename;
        std::string timestamp_str;

        if (use_timestamp) {
            // Generate timestamped filename
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm time_info;
            #ifdef _WIN32
                localtime_s(&time_info, &in_time_t);
            #else
                localtime_r(&in_time_t, &time_info);
            #endif

            std::stringstream ss;
            ss << std::put_time(&time_info, "%Y-%m-%d_%H-%M-%S");
            timestamp_str = ss.str();

            // Extract extension if present
            std::string name_part = base_filename;
            std::string ext = ".log";
            size_t dot_pos = base_filename.find_last_of('.');
            if (dot_pos != std::string::npos) {
                name_part = base_filename.substr(0, dot_pos);
                ext = base_filename.substr(dot_pos);
            }
            final_filename = name_part + "_" + timestamp_str + ext;
        }

        std::filesystem::path path = log_path;
        std::string new_filename;
        if (!path.empty()) {
            try {
                std::filesystem::create_directories(path);
            } catch (...) {
                // Ignore, let file open fail if necessary
            }
            new_filename = (path / final_filename).string();
        } else {
            new_filename = final_filename;
        }

        // If we are re-initializing to the SAME file, don't close and reopen (to avoid truncation)
        // Or if we must reopen, use append mode if use_timestamp is true to avoid losing startup lines
        // that happened in the same second.
        if (m_file.is_open() && m_filename == new_filename) {
            _LogNoLock("Logger re-initialized to same file. Continuing...", true);
            return;
        }

        if (m_file.is_open()) {
            m_file.close();
        }

        m_filename = new_filename;

        // Open mode: if timestamped, use append to be safe against multi-init in same second.
        // If not timestamped (tests), use trunc to ensure clean file.
        std::ios_base::openmode mode = std::ios::out;
        if (use_timestamp) {
            mode |= std::ios::app;
        } else {
            mode |= std::ios::trunc;
        }

        m_file.open(m_filename, mode);
        if (m_file.is_open()) {
            m_initialized = true;
            _LogNoLock("Logger Initialized. Version: " + std::string(LMUFFB_VERSION), true);
        }
    }

    void Close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file << "Logger Closed Explicitly.\n";
            m_file.close();
        }
        m_initialized = false;
    }

    void Log(const char* fmt, ...) {
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        StringUtils::vSafeFormat(buffer, sizeof(buffer), fmt, args);
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
        StringUtils::vSafeFormat(buffer, sizeof(buffer), fmt, args);
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
