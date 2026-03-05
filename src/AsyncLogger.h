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
#include <cstdint>   // For uint8_t
#include <lz4.h>     // For LZ4 compression

// Forward declaration
struct TelemInfoV01;
class FFBEngine;

// Log frame structure - captures one physics tick
#pragma pack(push, 1)
struct LogFrame {
    double timestamp;        // Time
    double delta_time;       // DeltaTime
    
    // --- PROCESSED 400Hz DATA (Smooth) ---
    float speed;             // Speed
    float lat_accel;         // LatAccel
    float long_accel;        // LongAccel
    float yaw_rate;          // YawRate
    
    float steering;          // Steering
    float throttle;          // Throttle
    float brake;             // Brake
    
    // --- RAW 100Hz GAME DATA (Step-function) ---
    float raw_steering;
    float raw_lat_accel;
    float raw_load_fl;
    float raw_load_fr;
    float raw_slip_vel_fl;
    float raw_slip_vel_fr;

    // --- ALGORITHM STATE (400Hz) ---
    float slip_angle_fl;     // SlipAngleFL
    float slip_angle_fr;     // SlipAngleFR
    float slip_ratio_fl;     // SlipRatioFL
    float slip_ratio_fr;     // SlipRatioFR
    float grip_fl;           // GripFL
    float grip_fr;           // GripFR
    float load_fl;           // LoadFL
    float load_fr;           // LoadFR
    float load_rl;           // LoadRL
    float load_rr;           // LoadRR

    float ride_height_fl;    // RideHeightFL
    float ride_height_fr;    // RideHeightFR
    float ride_height_rl;    // RideHeightRL
    float ride_height_rr;    // RideHeightRR
    float susp_deflection_fl;// SuspDeflectionFL
    float susp_deflection_fr;// SuspDeflectionFR
    float susp_deflection_rl;// SuspDeflectionRL
    float susp_deflection_rr;// SuspDeflectionRR
    
    float calc_slip_angle_front; // CalcSlipAngle
    float calc_grip_front;       // CalcGripFront
    float calc_grip_rear;        // CalcGripRear
    float grip_delta;            // GripDelta

    float raw_yaw_accel;         // RawYawAccel
    float smoothed_yaw_accel;    // SmoothedYawAccel
    float ffb_yaw_kick;          // FFBYawKick
    float lat_load_norm;         // LatLoadNorm
    
    float dG_dt;                 // dG_dt
    float dAlpha_dt;             // dAlpha_dt
    float slope_current;         // SlopeCurrent
    float slope_raw_unclamped;   // SlopeRaw
    float slope_numerator;       // SlopeNum
    float slope_denominator;     // SlopeDenom
    float hold_timer;            // HoldTimer
    float input_slip_smoothed;   // InputSlipSmooth
    float slope_smoothed;        // SlopeSmoothed
    float confidence;            // Confidence
    
    float surface_type_fl;       // SurfaceFL
    float surface_type_fr;       // SurfaceFR
    float slope_torque;          // SlopeTorque
    float slew_limited_g;        // SlewLimitedG
    
    // --- FINAL OUTPUTS (400Hz) ---
    float ffb_total;             // FFBTotal
    float ffb_base;              // FFBBase
    float ffb_shaft_torque;      // FFBShaftTorque
    float ffb_gen_torque;        // FFBGenTorque
    float ffb_sop;               // FFBSoP
    float ffb_grip_factor;       // GripFactor
    float speed_gate;            // SpeedGate
    float load_peak_ref;         // LoadPeakRef
    
    uint8_t clipping;            // Clipping
    uint8_t marker;              // Marker
};
#pragma pack(pop)

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
    bool torque_passthrough; // v0.7.63
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

        m_filename = path_prefix + "lmuffb_log_" + timestamp_str + "_" + car + "_" + track + ".bin";

        // Open file in binary mode
        m_file.open(m_filename, std::ios::out | std::ios::binary);
        if (m_file.is_open()) {
            WriteHeader(info);
            m_running = true;
            m_worker = std::thread(&AsyncLogger::WorkerThread, this);
        }
    }
    
    // Stop logging and flush
    void Stop() noexcept {
        try {
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
        } catch (...) {
            // Destructor/Stop should not throw
        }
    }
    
    // Log a frame - called from FFB thread (must be fast!)
    void Log(const LogFrame& frame) {
        if (!m_running) return;
        
        // Decimation: 400Hz -> 400Hz (v0.7.126: decimation removed, DECIMATION_FACTOR=1)
        if (++m_decimation_counter < DECIMATION_FACTOR && !frame.marker && !m_pending_marker) {
            return;
        }
        m_decimation_counter = 0; // Reset counter to prevent overflow (Review fix)

        LogFrame f = frame;
        if (m_pending_marker) {
            f.marker = 1;
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

    // Toggle compression
    void EnableCompression(bool enable) { m_lz4_enabled = enable; }
    
    // Status getters
    bool IsLogging() const { return m_running; }
    size_t GetFrameCount() const { return m_frame_count; }
    std::string GetFilename() const { return m_filename; }
    size_t GetFileSizeBytes() const { return m_file_size_bytes; }

private:
    AsyncLogger() : m_running(false), m_pending_marker(false), m_frame_count(0), m_decimation_counter(0), 
                    m_file_size_bytes(0), m_last_flush_time(std::chrono::steady_clock::now()),
                    m_lz4_enabled(false) {}
    ~AsyncLogger() { Stop(); }
    
    // No copy
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    
    void WorkerThread() {
        std::vector<char> compressed_buffer;
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
            if (!m_buffer_writing.empty()) {
                size_t src_size = m_buffer_writing.size() * sizeof(LogFrame);
                const char* src_ptr = reinterpret_cast<const char*>(m_buffer_writing.data());

                if (m_lz4_enabled) {
                    int max_dst_size = LZ4_compressBound((int)src_size);
                    compressed_buffer.resize(max_dst_size);

                    int compressed_size = LZ4_compress_default(src_ptr, compressed_buffer.data(), (int)src_size, max_dst_size);

                    if (compressed_size > 0) {
                        // Write block size header (compressed, then uncompressed)
                        uint32_t header[2] = { (uint32_t)compressed_size, (uint32_t)src_size };
                        m_file.write(reinterpret_cast<const char*>(header), 8);
                        // Write payload
                        m_file.write(compressed_buffer.data(), compressed_size);
                        m_file_size_bytes += (8 + compressed_size);
                    }
                } else {
                    m_file.write(src_ptr, src_size);
                    m_file_size_bytes += src_size;
                }
                m_buffer_writing.clear();
            }
            
            // Periodic flush to minimize data loss on crash
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_flush_time).count();
            if (elapsed >= FLUSH_INTERVAL_SECONDS) {
                m_file.flush();
                m_last_flush_time = now;
            }
            
        }
    }

    void WriteHeader(const SessionInfo& info) {
        m_file << "# LMUFFB Telemetry Log v1.0\n";
        m_file << "# App Version: " << info.app_version << "\n";
        m_file << "# Compression: " << (m_lz4_enabled ? "LZ4" : "None") << "\n";
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
        m_file << "# Torque Passthrough: " << (info.torque_passthrough ? "Enabled" : "Disabled") << "\n";
        m_file << "# ========================\n";
        
        // CSV Header for human readability and test compatibility
        m_file << "# Fields: Time,DeltaTime,Speed,LatAccel,LongAccel,YawRate,Steering,Throttle,Brake,"
               << "RawSteering,RawLatAccel,RawLoadFL,RawLoadFR,RawSlipVelFL,RawSlipVelFR,"
               << "SlipAngleFL,SlipAngleFR,SlipRatioFL,SlipRatioFR,GripFL,GripFR,"
               << "LoadFL,LoadFR,LoadRL,LoadRR,"
               << "RideHeightFL,RideHeightFR,RideHeightRL,RideHeightRR,"
               << "SuspDeflectionFL,SuspDeflectionFR,SuspDeflectionRL,SuspDeflectionRR,"
               << "CalcSlipAngle,CalcGripFront,CalcGripRear,GripDelta,"
               << "RawYawAccel,SmoothedYawAccel,FFBYawKick,LatLoadNorm,"
               << "dG_dt,dAlpha_dt,SlopeCurrent,SlopeRaw,SlopeNum,SlopeDenom,HoldTimer,InputSlipSmooth,SlopeSmoothed,Confidence,"
               << "SurfaceFL,SurfaceFR,SlopeTorque,SlewLimitedG,"
               << "FFBTotal,FFBBase,FFBShaftTorque,FFBGenTorque,FFBSoP,GripFactor,SpeedGate,LoadPeakRef,Clipping,Marker\n";
        m_file << "[DATA_START]\n";
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
    
    bool m_lz4_enabled; // Internal compression toggle

    static const int DECIMATION_FACTOR = 1; // 400Hz -> 400Hz
    static const size_t BUFFER_THRESHOLD = 200; // ~0.5s of data
    static const int FLUSH_INTERVAL_SECONDS = 5; // Flush every 5 seconds
};

#endif // ASYNCLOGGER_H
