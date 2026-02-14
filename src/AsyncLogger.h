#ifndef ASYNCLOGGER_H
#define ASYNCLOGGER_H

#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm> // For std::max
#include <filesystem>

// Forward declaration
struct TelemInfoV01;
class FFBEngine;

// Log frame structure - captures one physics tick
struct LogFrame {
    double timestamp;
    double delta_time;
    
    // Driver Inputs
    float steering;
    float throttle;
    float brake;
    
    // Vehicle State
    float speed;             // m/s
    float lat_accel;         // m/s²
    float long_accel;        // m/s²
    float yaw_rate;          // rad/s
    
    // Front Axle - Raw Telemetry
    float slip_angle_fl;
    float slip_angle_fr;
    float slip_ratio_fl;
    float slip_ratio_fr;
    float grip_fl;
    float grip_fr;
    float load_fl;
    float load_fr;
    
    // Front Axle - Calculated
    float calc_slip_angle_front;
    float calc_grip_front;
    
    // Slope Detection Specific
    float dG_dt;             // Derivative of lateral G
    float dAlpha_dt;         // Derivative of slip angle
    float slope_current;     // dG/dAlpha ratio
    float slope_raw_unclamped; // NEW v0.7.38
    float slope_numerator;     // NEW v0.7.38
    float slope_denominator;   // NEW v0.7.38
    float hold_timer;          // NEW v0.7.38
    float input_slip_smoothed; // NEW v0.7.38
    float slope_smoothed;    // Smoothed grip output
    float confidence;        // Confidence factor (v0.7.3)
    float surface_type_fl;   // NEW v0.7.39
    float surface_type_fr;   // NEW v0.7.39
    float slope_torque;      // NEW v0.7.40
    float slew_limited_g;    // NEW v0.7.40
    
    // Rear Axle
    float calc_grip_rear;
    float grip_delta;        // Front - Rear
    
    // FFB Output
    float ffb_total;         // Normalized output
    float ffb_base;          // Base steering shaft force
    float ffb_sop;           // Seat of Pants force
    float ffb_grip_factor;   // Applied grip modulation
    float speed_gate;        // Speed gate factor
    float load_peak_ref;     // NEW: Dynamic normalization reference
    bool clipping;           // Output clipping flag
    
    // User Markers
    bool marker;             // User-triggered marker
};

// Session metadata for header
struct SessionInfo {
    std::string driver_name;
    std::string vehicle_name;
    std::string track_name;
    std::string app_version;
    
    // Key settings snapshot
    float gain;
    float understeer_effect;
    float sop_effect;
    bool slope_enabled;
    float slope_sensitivity;
    float slope_threshold;
    float slope_alpha_threshold;
    float slope_decay_rate;
};

class AsyncLogger {
public:
    static AsyncLogger& Get() {
        static AsyncLogger instance;
        return instance;
    }

    // Start logging - called from GUI
    void Start(const SessionInfo& info, const std::string& base_path = "") {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_running) return;

        m_buffer_active.reserve(BUFFER_THRESHOLD * 2);
        m_buffer_writing.reserve(BUFFER_THRESHOLD * 2);
        m_frame_count = 0;
        m_pending_marker = false;
        m_decimation_counter = 0;

        // Generate filename
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        // Use localtime_s for thread safety (MSVC)
        std::tm time_info;
        #ifdef _WIN32
            localtime_s(&time_info, &in_time_t);
        #else
            localtime_r(&in_time_t, &time_info);
        #endif
        
        std::stringstream ss;
        ss << std::put_time(&time_info, "%Y-%m-%d_%H-%M-%S");
        std::string timestamp_str = ss.str();
        
        std::string car = SanitizeFilename(info.vehicle_name);
        std::string track = SanitizeFilename(info.track_name);
        
        std::string path_prefix = base_path;
        if (!path_prefix.empty()) {
            // Ensure directory exists
            try {
                std::filesystem::create_directories(path_prefix);
            } catch (...) {
                // Ignore, let file open fail if necessary
            }
            
            if (path_prefix.back() != '/' && path_prefix.back() != '\\') {
                 path_prefix += "/";
            }
        }

        m_filename = path_prefix + "lmuffb_log_" + timestamp_str + "_" + car + "_" + track + ".csv";

        // Open file
        m_file.open(m_filename);
        if (m_file.is_open()) {
            WriteHeader(info);
            m_running = true;
            m_worker = std::thread(&AsyncLogger::WorkerThread, this);
        }
    }
    
    // Stop logging and flush
    void Stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running) return;
            m_running = false;
        }
        m_cv.notify_one();
        if (m_worker.joinable()) {
            m_worker.join();
        }
        if (m_file.is_open()) {
            m_file.close();
        }
        m_buffer_active.clear();
        m_buffer_writing.clear();
    }
    
    // Log a frame - called from FFB thread (must be fast!)
    void Log(const LogFrame& frame) {
        if (!m_running) return;
        
        // Decimation: 400Hz -> 100Hz
        if (++m_decimation_counter < DECIMATION_FACTOR && !frame.marker && !m_pending_marker) {
            return;
        }
        m_decimation_counter = 0;

        LogFrame f = frame;
        if (m_pending_marker) {
            f.marker = true;
            m_pending_marker = false;
        }

        bool should_notify = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running) return; 
            m_buffer_active.push_back(f);
            should_notify = (m_buffer_active.size() >= BUFFER_THRESHOLD);
        }
        
        m_frame_count++;
        
        if (should_notify) {
             m_cv.notify_one();
        }
    }
    
    // Trigger a user marker
    void SetMarker() { m_pending_marker = true; }
    
    // Status getters
    bool IsLogging() const { return m_running; }
    size_t GetFrameCount() const { return m_frame_count; }
    std::string GetFilename() const { return m_filename; }
    size_t GetFileSizeBytes() const { return m_file_size_bytes; }

private:
    AsyncLogger() : m_running(false), m_pending_marker(false), m_frame_count(0), m_decimation_counter(0), 
                    m_file_size_bytes(0), m_last_flush_time(std::chrono::steady_clock::now()) {}
    ~AsyncLogger() { Stop(); }
    
    // No copy
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    
    void WorkerThread() {
        while (true) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_running || !m_buffer_active.empty(); });
            
            // Swap buffers
            if (!m_buffer_active.empty()) {
                std::swap(m_buffer_active, m_buffer_writing);
            }
            
            // If stopped and empty, exit
            if (!m_running && m_buffer_writing.empty()) {
                 break;
            }
            
            lock.unlock();
            
            // Write buffer to disk
            for (const auto& frame : m_buffer_writing) {
                WriteFrame(frame);
            }
            m_buffer_writing.clear();
            
            // Periodic flush to minimize data loss on crash
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_flush_time).count();
            if (elapsed >= FLUSH_INTERVAL_SECONDS) {
                m_file.flush();
                m_last_flush_time = now;
            }
            
            if (!m_running) break;
        }
    }

    void WriteHeader(const SessionInfo& info) {
        m_file << "# LMUFFB Telemetry Log v1.0\n";
        m_file << "# App Version: " << info.app_version << "\n";
        m_file << "# ========================\n";
        m_file << "# Session Info\n";
        m_file << "# ========================\n";
        m_file << "# Driver: " << info.driver_name << "\n";
        m_file << "# Vehicle: " << info.vehicle_name << "\n";
        m_file << "# Track: " << info.track_name << "\n";
        m_file << "# ========================\n";
        m_file << "# FFB Settings\n";
        m_file << "# ========================\n";
        m_file << "# Gain: " << info.gain << "\n";
        m_file << "# Understeer Effect: " << info.understeer_effect << "\n";
        m_file << "# SoP Effect: " << info.sop_effect << "\n";
        m_file << "# Slope Detection: " << (info.slope_enabled ? "Enabled" : "Disabled") << "\n";
        m_file << "# Slope Sensitivity: " << info.slope_sensitivity << "\n";
        m_file << "# Slope Threshold: " << info.slope_threshold << "\n";
        m_file << "# Slope Alpha Threshold: " << info.slope_alpha_threshold << "\n";
        m_file << "# Slope Decay Rate: " << info.slope_decay_rate << "\n";
        m_file << "# ========================\n";
        
        // CSV Header
        m_file << "Time,DeltaTime,Speed,LatAccel,LongAccel,YawRate,Steering,Throttle,Brake,"
               << "SlipAngleFL,SlipAngleFR,SlipRatioFL,SlipRatioFR,GripFL,GripFR,LoadFL,LoadFR,"
               << "CalcSlipAngle,CalcGripFront,CalcGripRear,GripDelta,"
               << "dG_dt,dAlpha_dt,SlopeCurrent,SlopeRaw,SlopeNum,SlopeDenom,HoldTimer,InputSlipSmooth,SlopeSmoothed,Confidence,"
               << "SurfaceFL,SurfaceFR,SlopeTorque,SlewLimitedG,"
               << "FFBTotal,FFBBase,FFBSoP,GripFactor,SpeedGate,LoadPeakRef,Clipping,Marker\n";
    }

    void WriteFrame(const LogFrame& frame) {
        m_file << std::fixed << std::setprecision(4)
               << frame.timestamp << "," << frame.delta_time << "," 
               << frame.speed << "," << frame.lat_accel << "," << frame.long_accel << "," << frame.yaw_rate << ","
               << frame.steering << "," << frame.throttle << "," << frame.brake << ","
               
               << frame.slip_angle_fl << "," << frame.slip_angle_fr << "," 
               << frame.slip_ratio_fl << "," << frame.slip_ratio_fr << ","
               << frame.grip_fl << "," << frame.grip_fr << ","
               << frame.load_fl << "," << frame.load_fr << ","
               
               << frame.calc_slip_angle_front << "," << frame.calc_grip_front << "," << frame.calc_grip_rear << "," << frame.grip_delta << ","
               
               << frame.dG_dt << "," << frame.dAlpha_dt << "," << frame.slope_current << ","
               << frame.slope_raw_unclamped << "," << frame.slope_numerator << "," << frame.slope_denominator << ","
               << frame.hold_timer << "," << frame.input_slip_smoothed << ","
               << frame.slope_smoothed << "," << frame.confidence << ","
               << frame.surface_type_fl << "," << frame.surface_type_fr << ","
               << frame.slope_torque << "," << frame.slew_limited_g << ","
               
               << frame.ffb_total << "," << frame.ffb_base << "," << frame.ffb_sop << "," 
               << frame.ffb_grip_factor << "," << frame.speed_gate << "," << frame.load_peak_ref << ","
               << (frame.clipping ? 1 : 0) << "," << (frame.marker ? 1 : 0) << "\n";
        
        // Track file size for monitoring
        m_file_size_bytes += 200; // Approximate bytes per line
    }

    std::string SanitizeFilename(const std::string& input) {
        std::string out = input;
        // Replace invalid Windows filename characters
        std::replace(out.begin(), out.end(), ' ', '_');
        std::replace(out.begin(), out.end(), '/', '_');
        std::replace(out.begin(), out.end(), '\\', '_');
        std::replace(out.begin(), out.end(), ':', '_');
        std::replace(out.begin(), out.end(), '*', '_');
        std::replace(out.begin(), out.end(), '?', '_');
        std::replace(out.begin(), out.end(), '"', '_');
        std::replace(out.begin(), out.end(), '<', '_');
        std::replace(out.begin(), out.end(), '>', '_');
        std::replace(out.begin(), out.end(), '|', '_');
        return out;
    }
    
    std::ofstream m_file;
    std::string m_filename;
    std::thread m_worker;
    
    std::vector<LogFrame> m_buffer_active;
    std::vector<LogFrame> m_buffer_writing;
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;
    std::atomic<bool> m_pending_marker;
    std::atomic<size_t> m_frame_count;
    
    int m_decimation_counter;
    std::atomic<size_t> m_file_size_bytes;
    std::chrono::steady_clock::time_point m_last_flush_time;
    
    static const int DECIMATION_FACTOR = 4; // 400Hz -> 100Hz
    static const size_t BUFFER_THRESHOLD = 200; // ~0.5s of data
    static const int FLUSH_INTERVAL_SECONDS = 5; // Flush every 5 seconds
};

#endif // ASYNCLOGGER_H
