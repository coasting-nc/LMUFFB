# Implementation Plan: Telemetry Logger for FFB Diagnostics

## 1. Context

**Goal:** Implement a high-performance asynchronous telemetry logging system to capture FFB calculations and telemetry data for debugging slope detection and other FFB effects. The logger must not impact the 400Hz FFB loop performance.

**Version:** This feature is part of the v0.7.x diagnostic tooling  
**Date:** 2026-02-03  
**Priority:** Medium (Diagnostic infrastructure)

## 2. Reference Documents

- Design Proposal: `docs/dev_docs/design proposal for a High-Performance Asynchronous Telemetry Logger.md`
- Investigation Report: `docs/dev_docs/investigations/slope_detection_issues_post_v071_investigation.md`
- Telemetry Investigation: `docs/dev_docs/telemetry_logging_investigation.md`

---

## 3. Codebase Analysis Summary

### 3.1 Current Architecture Overview

The FFB system operates at 400Hz in a dedicated thread:

```
Main Thread (GUI)
    │
    └── FFBPlugin Thread (400Hz)
            │
            ├── Read Telemetry (SharedMemory)
            ├── FFBEngine::calculate_force()
            └── Output to DirectInput/vJoy
```

**Key constraint:** Any I/O in the FFB thread will cause stuttering. Disk writes can take 1-100ms, which would cause noticeable FFB drops.

### 3.2 Available Telemetry Data

From `rF2Data.h` and `InternalsPlugin.hpp`:

| Field | Type | Description |
|-------|------|-------------|
| `mVehicleName[64]` | char[] | Current vehicle name |
| `mTrackName[64]` | char[] | Current track name |
| `mDriverName[32]` | char[] | Driver name |
| `mElapsedTime` | double | Session elapsed time |
| `mDeltaTime` | double | Frame delta time |
| `mLocalVel` | Vec3 | Local velocity (speed) |
| `mLocalAccel` | Vec3 | Local acceleration |
| `mWheel[4]` | WheelV01 | Per-wheel data (slip, load, etc.) |

### 3.3 Impacted Components

| Component | Impact | Description |
|-----------|--------|-------------|
| `FFBEngine.h` | Modified | Add logging call at end of `calculate_force()` |
| `GuiLayer.cpp` | Modified | Add logging toggle button and status display |
| `Config.cpp` | Modified | Persist logging preferences |
| **NEW** `AsyncLogger.h` | Created | New header-only logger class |

---

## 4. Proposed Architecture

### 4.1 Double-Buffered Asynchronous Design

```
┌─────────────────────────────────────────────────────────────────────┐
│ FFB Thread (400Hz)                                                  │
│                                                                     │
│   calculate_force() ──► LogFrame ──► AsyncLogger::Log()             │
│                                           │                         │
│                                           ▼                         │
│                                    ┌──────────────┐                 │
│                                    │ Active Buffer│ (fast push)     │
│                                    └──────────────┘                 │
└─────────────────────────────────────────────────────────────────────┘
                                           │
                                    (buffer swap)
                                           │
                                           ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Logger Worker Thread (async)                                        │
│                                                                     │
│   Wait for signal ──► Swap buffers ──► Write to CSV file            │
│                                           │                         │
│                                           ▼                         │
│                                    ┌──────────────┐                 │
│                                    │ Write Buffer │ (slow I/O)      │
│                                    └──────────────┘                 │
└─────────────────────────────────────────────────────────────────────┘
```

### 4.2 Log File Structure

#### 4.2.1 Filename Convention

```
lmuffb_log_<DATE>_<TIME>_<CAR>_<TRACK>.csv
```

Example:
```
lmuffb_log_2026-02-03_14-30-00_Porsche_911_GT3_R_Spa-Francorchamps.csv
```

#### 4.2.2 Header Section (Session Metadata)

```csv
# LMUFFB Telemetry Log v1.0
# Date: 2026-02-03 14:30:00
# App Version: 0.7.3
# ========================
# Session Info
# ========================
# Driver: PlayerName
# Vehicle: Porsche 911 GT3 R
# Track: Spa-Francorchamps
# ========================
# FFB Settings
# ========================
# Gain: 1.0
# Understeer Effect: 1.0
# SoP Effect: 1.666
# Slope Detection: Enabled
# Slope Sensitivity: 0.5
# Slope Threshold: -0.3
# Slope Alpha Threshold: 0.02
# Slope Decay Rate: 5.0
# ========================
Time,DeltaTime,Speed,LatAccel,LongAccel,Steering,...
0.0000,0.0025,41.67,0.52,-0.10,0.15,...
```

---

## 5. Proposed Changes

### 5.1 New File: `src/AsyncLogger.h`

Header-only implementation of the asynchronous logger.

```cpp
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
    void Start(const SessionInfo& info, const std::string& base_path = "");
    
    // Stop logging and flush
    void Stop();
    
    // Log a frame - called from FFB thread (must be fast!)
    void Log(const LogFrame& frame);
    
    // Trigger a user marker
    void SetMarker() { m_pending_marker = true; }
    
    // Status getters
    bool IsLogging() const { return m_running; }
    size_t GetFrameCount() const { return m_frame_count; }
    std::string GetFilename() const { return m_filename; }

private:
    AsyncLogger();
    ~AsyncLogger() { Stop(); }
    
    // No copy
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    
    void WorkerThread();
    void WriteHeader(const SessionInfo& info);
    void WriteFrame(const LogFrame& frame);
    std::string SanitizeFilename(const std::string& input);
    
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
```

### 5.2 FFBEngine.h Changes

#### 5.2.1 Add Logging Integration

**Location:** At the end of `calculate_force()` function (before return)

```cpp
// Telemetry Logging (v0.7.x)
if (AsyncLogger::Get().IsLogging()) {
    LogFrame frame;
    frame.timestamp = data->mElapsedTime;
    frame.delta_time = data->mDeltaTime;
    
    // Inputs
    frame.steering = (float)data->mUnfilteredSteering;
    frame.throttle = (float)data->mUnfilteredThrottle;
    frame.brake = (float)data->mUnfilteredBrake;
    
    // Vehicle state
    frame.speed = (float)ctx.car_speed;
    frame.lat_accel = (float)data->mLocalAccel.x;
    frame.long_accel = (float)data->mLocalAccel.z;
    frame.yaw_rate = (float)data->mLocalRot.y;
    
    // Front axle raw
    frame.slip_angle_fl = (float)fl.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
    frame.slip_angle_fr = (float)fr.mLateralPatchVel / (float)(std::max)(1.0, ctx.car_speed);
    frame.grip_fl = (float)fl.mGripFract;
    frame.grip_fr = (float)fr.mGripFract;
    frame.load_fl = (float)fl.mTireLoad;
    frame.load_fr = (float)fr.mTireLoad;
    
    // Calculated values
    frame.calc_slip_angle_front = (float)m_grip_diag.front_slip_angle;
    frame.calc_grip_front = (float)ctx.avg_grip;
    
    // Slope detection
    frame.dG_dt = (float)m_slope_dG_dt;           // Need to expose these
    frame.dAlpha_dt = (float)m_slope_dAlpha_dt;   // Need to expose these
    frame.slope_current = (float)m_slope_current;
    frame.slope_smoothed = (float)m_slope_smoothed_output;
    
    // Rear axle
    frame.calc_grip_rear = (float)ctx.avg_rear_grip;
    frame.grip_delta = (float)(ctx.avg_grip - ctx.avg_rear_grip);
    
    // FFB output
    frame.ffb_total = (float)norm_force;
    frame.ffb_grip_factor = (float)ctx.grip_factor;
    frame.ffb_sop = (float)ctx.sop_base_force;
    frame.speed_gate = (float)ctx.speed_gate;
    frame.clipping = (std::abs(norm_force) > 0.99);
    
    AsyncLogger::Get().Log(frame);
}
```

#### 5.2.2 Expose Intermediate Values

**Location:** Member variables section (after line 320)

```cpp
// Logging intermediate values (exposed for AsyncLogger)
double m_slope_dG_dt = 0.0;       // Last calculated dG/dt
double m_slope_dAlpha_dt = 0.0;   // Last calculated dAlpha/dt
```

**Location:** In `calculate_slope_grip()` function

```cpp
double dG_dt = calculate_sg_derivative(...);
double dAlpha_dt = calculate_sg_derivative(...);

// Store for logging
m_slope_dG_dt = dG_dt;
m_slope_dAlpha_dt = dAlpha_dt;
```

### 5.3 GuiLayer.cpp Changes

#### 5.3.1 Add Logging Controls

**Location:** In the Troubleshooting/Debug section

```cpp
// ===== TELEMETRY LOGGING =====
ImGui::Separator();
ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Telemetry Logging");

if (AsyncLogger::Get().IsLogging()) {
    // Recording state
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Stop Recording", ImVec2(150, 30))) {
        AsyncLogger::Get().Stop();
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "RECORDING");
    
    // Frame count
    ImGui::Text("Frames: %zu | File: %s", 
        AsyncLogger::Get().GetFrameCount(),
        AsyncLogger::Get().GetFilename().c_str());
    
    // Marker button
    if (ImGui::Button("Mark Event [Space]", ImVec2(150, 25))) {
        AsyncLogger::Get().SetMarker();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Use to mark interesting moments)");
    
} else {
    // Idle state
    if (ImGui::Button("Start Recording", ImVec2(150, 30))) {
        // Build session info
        SessionInfo info;
        info.driver_name = current_driver_name;  // Need to capture this
        info.vehicle_name = current_vehicle_name;
        info.track_name = current_track_name;
        info.app_version = VERSION_STRING;
        
        info.gain = engine.m_gain;
        info.understeer_effect = engine.m_understeer_effect;
        info.sop_effect = engine.m_sop_effect;
        info.slope_enabled = engine.m_slope_detection_enabled;
        info.slope_sensitivity = engine.m_slope_sensitivity;
        info.slope_threshold = engine.m_slope_negative_threshold;
        
        AsyncLogger::Get().Start(info);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Records FFB data for analysis");
}
```

### 5.4 Config Changes (Optional)

#### 5.4.1 Persist Logging Preferences

```cpp
// In Config.h
static std::string m_log_output_path;  // Custom log output directory
static bool m_log_autostart;           // Auto-start logging on connect

// In Config.cpp Save/Load
file << "log_output_path=" << Config::m_log_output_path << "\n";
file << "log_autostart=" << Config::m_log_autostart << "\n";
```

---

## 6. Data Channels Specification

### 6.1 Core Channels (Always Logged)

| Channel | Column Name | Unit | Description |
|---------|-------------|------|-------------|
| Timestamp | `Time` | seconds | Session elapsed time |
| Frame Delta | `DeltaTime` | seconds | Time since last frame |
| Speed | `Speed` | m/s | Vehicle speed |
| Lateral Accel | `LatAccel` | m/s² | Lateral acceleration |
| Steering Input | `Steering` | -1 to 1 | Unfiltered steering input |

### 6.2 Slope Detection Channels

| Channel | Column Name | Unit | Description |
|---------|-------------|------|-------------|
| dG/dt | `dG_dt` | G/s | Rate of change of lateral G |
| dAlpha/dt | `dAlpha_dt` | rad/s | Rate of change of slip angle |
| Slope Current | `SlopeCurrent` | G/rad | dG/dAlpha ratio |
| Slope Smoothed | `SlopeSmoothed` | 0-1 | Smoothed grip output |
| Confidence | `Confidence` | 0-1 | Confidence factor (v0.7.3) |

### 6.3 FFB Output Channels

| Channel | Column Name | Unit | Description |
|---------|-------------|------|-------------|
| FFB Total | `FFBTotal` | -1 to 1 | Normalized output |
| Grip Factor | `GripFactor` | 0-1 | Applied grip modulation |
| Clipping | `Clipping` | 0/1 | Output clipping flag |
| Marker | `Marker` | 0/1 | User marker |

---

## 7. Test Plan (TDD-Ready)

### 7.1 Unit Tests

#### Test 1: `test_logger_start_stop`
**Description:** Verify logger can start and stop without crashes.

**Assertions:**
- `IsLogging()` returns false initially
- After `Start()`, `IsLogging()` returns true
- After `Stop()`, `IsLogging()` returns false
- File is created with correct name

#### Test 2: `test_logger_frame_logging`
**Description:** Verify frames are logged correctly.

**Setup:** Start logger, log 10 frames, stop logger.

**Assertions:**
- `GetFrameCount() == 10`
- CSV file contains header + 10+ data rows (accounting for decimation)

#### Test 3: `test_logger_decimation`
**Description:** Verify 400Hz -> 100Hz decimation works.

**Setup:** Log 400 frames.

**Assertions:**
- CSV file contains ~100 data rows (400 / 4)

#### Test 4: `test_logger_marker`
**Description:** Verify user markers are recorded.

**Setup:** Log frames, trigger marker, log more frames.

**Assertions:**
- At least one row has `Marker=1`

#### Test 5: `test_logger_filename_sanitization`
**Description:** Verify special characters are removed from filenames.

**Test Cases:**
- `Porsche 911 GT3 R` → `Porsche_911_GT3_R`
- `Track: Spa/Belgium` → `Track_Spa_Belgium`

### 7.2 Performance Tests

#### Test 6: `test_logger_performance_impact`
**Description:** Verify logging doesn't slow down FFB loop.

**Setup:** Measure time of 1000 `calculate_force()` calls with and without logging.

**Assertions:**
- Overhead per call < 10 microseconds
- No significant increase in max call time

### 7.3 Test Count

**Baseline + 6 new tests**

---

## 8. Deliverables Checklist

### 8.1 Code Changes
- [ ] Create `src/AsyncLogger.h` with full implementation
- [ ] Modify `src/FFBEngine.h` to expose slope intermediate values
- [ ] Modify `src/FFBEngine.h` to call logger at end of `calculate_force()`
- [ ] Modify `src/GuiLayer.cpp` to add logging UI controls
- [ ] Optionally modify `src/Config.h/cpp` for persistence

### 8.2 Tests
- [ ] Add 6 new tests to `tests/test_ffb_engine.cpp`
- [ ] All tests pass

### 8.3 Documentation
- [ ] Update `USER_CHANGELOG.md` with logging feature
- [ ] Create `docs/diagnostics/how_to_use_telemetry_logging.md`
- [ ] Update this plan with implementation notes

## 8.4 Implementation Notes

**Unforeseen Issues:**
- **Compiler Warning C4996 (strncpy)**: MSVC 2022 flagged `strncpy` as unsafe. Updated to `strncpy_s` in both `FFBEngine.h` and `GuiLayer.cpp` to resolve build errors in newer toolchains.
- **ImGui String Buffer Management**: Handling the dynamic `log_path` in ImGui required creating a local char buffer and using `strncpy_s` to safely update the `std::string` in `Config`.
- **Directory Creation Permission**: Discovered that `std::filesystem::create_directories` can fail if the app doesn't have write permissions in the root folder. Added a try-catch block to prevent crashes, letting the subsequent file open operation handle the actual failure reporting.

**Plan Deviations:**
- **Vehicle/Track Name Location**: Instead of passing names every frame (which would bloat the 400Hz API), I integrated string capture directly into `FFBEngine::calculate_force`. It now only copies the strings when it detects a mismatch (e.g., car change or new session), which is much more efficient.
- **Persistence Bonus**: Integrated `auto_start_logging` into the global `Config` persistence system, which wasn't strictly required by the initial MVP but provides significant UX value.
- **Decimation logic modification**: Initial plan suggested 100Hz fixed. Implementation uses a `DECIMATION_FACTOR` which is easier to tune if users want higher/lower fidelity later.

**Challenges Encountered:**
- **Thread Safety in Auto-Start**: Coordinating the session detection in `main.cpp` (FFB Thread) with settings loaded by the GUI thread required careful use of `g_engine_mutex` to avoid race conditions when capturing the initial `SessionInfo`.
- **UTF-8 File Encoding**: Encountered some issues with file reading tools when `config.ini` was saved in specific encodings. Standardized all string handling to UTF-8.

**Recommendations for Future Plans:**
- **Binary Logging Option**: For long endurance races, CSV files can grow quite large (~10MB/hr). Future versions should consider a compressed binary format (e.g., Protobuf or custom flatbuffer) to reduce disk footprint.
- **Live Telemetry Overlay**: The logging infrastructure could be reused to feed a live performance overlay in the GUI, reducing the need for post-processing of CSVs for quick tuning.

---

## 9. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Logger blocks FFB thread | Low | High | Double-buffering, mutex only on swap |
| Huge log files | Medium | Low | Decimation (100Hz), session-based files |
| Filename conflicts | Low | Low | Timestamp in filename |
| Memory exhaustion | Low | Medium | Buffer size limits, periodic flush |

---

*Plan Created: 2026-02-03*
*Status: Ready for TDD Implementation*
