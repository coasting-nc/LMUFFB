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
    float slope_smoothed;    // Smoothed grip output
    float confidence;        // Confidence factor (v0.7.3)
    
    // Rear Axle
    float calc_grip_rear;
    float grip_delta;        // Front - Rear
    
    // FFB Output
    float ffb_total;         // Normalized output
    float ffb_base;          // Base steering shaft force
    float ffb_sop;           // Seat of Pants force
    float ffb_grip_factor;   // Applied grip modulation
    float speed_gate;        // Speed gate factor
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
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
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
            f.marker = true; // Use passed frame combined with marker? Or previous frame? 
            // The passed frame is current.
            // But if pending marker was set asynchronously, we tag THIS frame.
            m_pending_marker = false;
        }
        // Also if frame.marker was already true, we keep it true.
        if (f.marker) {
            // Nothing to do
        } else if (m_pending_marker) { // Double check logic
             // I already checked m_pending_marker above. 
             // "if (m_pending_marker) { f.marker = true; ... }"
             // Wait, I used `f` vs `frame`. `LogFrame f = frame;`
             // Code above:
             /*
                LogFrame f = frame;
                if (m_pending_marker) {
                    f.marker = true;
                    m_pending_marker = false;
                }
             */
             // This is correct.
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running) return; 
            m_buffer_active.push_back(f);
        }
        
        m_frame_count++;
        
        if (m_buffer_active.size() >= BUFFER_THRESHOLD) {
             m_cv.notify_one();
        }
    }
    
    // Trigger a user marker
    void SetMarker() { m_pending_marker = true; }
    
    // Status getters
    bool IsLogging() const { return m_running; }
    size_t GetFrameCount() const { return m_frame_count; }
    std::string GetFilename() const { return m_filename; }

private:
    AsyncLogger() : m_running(false), m_pending_marker(false), m_frame_count(0), m_decimation_counter(0) {}
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
               << "dG_dt,dAlpha_dt,SlopeCurrent,SlopeSmoothed,Confidence,"
               << "FFBTotal,FFBBase,FFBSoP,GripFactor,SpeedGate,Clipping,Marker\n";
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
               
               << frame.dG_dt << "," << frame.dAlpha_dt << "," << frame.slope_current << "," << frame.slope_smoothed << "," << frame.confidence << ","
               
               << frame.ffb_total << "," << frame.ffb_base << "," << frame.ffb_sop << "," 
               << frame.ffb_grip_factor << "," << frame.speed_gate << "," 
               << (frame.clipping ? 1 : 0) << "," << (frame.marker ? 1 : 0) << "\n";
    }

    std::string SanitizeFilename(const std::string& input) {
        std::string out = input;
        std::replace(out.begin(), out.end(), ' ', '_');
        std::replace(out.begin(), out.end(), '/', '_');
        std::replace(out.begin(), out.end(), '\\', '_');
        std::replace(out.begin(), out.end(), ':', '_');
        // Remove other invalid chars if needed
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
    static const int DECIMATION_FACTOR = 4; // 400Hz -> 100Hz
    static const size_t BUFFER_THRESHOLD = 200; // ~0.5s of data
};

#endif // ASYNCLOGGER_H
