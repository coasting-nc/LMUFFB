This is a critical feature for stabilizing the new physics workarounds. Since we are now *calculating* physics rather than just reading them, we need to see the math in action over time to tune it.

Here is the design proposal for a **High-Performance Asynchronous Telemetry Logger**.

### 1. Architectural Constraints
*   **The Golden Rule:** You **cannot** write to disk inside the `FFBThread` (400Hz). Disk I/O is blocking and unpredictable (can take 1ms or 100ms). Doing so will cause the FFB to stutter.
*   **The Solution:** **Double-Buffered Asynchronous Logging**.
    1.  **Producer (FFB Thread):** Writes data to a fast in-memory buffer (RAM).
    2.  **Consumer (Worker Thread):** Wakes up periodically, swaps the buffer, and writes the data to disk (CSV).

### 2. Data Format: CSV (Comma Separated Values)
While binary is faster, **CSV** is the right choice here because:
1.  **Universal:** Opens in Excel, Google Sheets.
2.  **MegaLogViewer:** Can be imported directly into tools like MegaLogViewer (used by tuners) or Motec i2 (via converters).
3.  **Human Readable:** You can open it in Notepad to check if a value is exactly `0.000`.

### 3. Implementation Design

#### A. The Data Structure (`LogFrame`)
We need a struct that captures the exact state of a physics tick.

```cpp
struct LogFrame {
    double timestamp;      // Time since session start
    
    // Inputs
    float steering_torque;
    float throttle;
    float brake;
    
    // Raw Telemetry (The "Truth")
    float raw_load_fl;
    float raw_grip_fl;
    float raw_susp_force_fl;
    float raw_ride_height_fl;
    float raw_lat_vel;
    
    // Calculated Physics (The "Workaround")
    float calc_load_fl;
    float calc_grip_fl;
    float calc_slip_ratio_fl;
    float calc_slip_angle_fl;
    
    // FFB Outputs (The Result)
    float ffb_total;
    float ffb_sop;
    float ffb_road;
    float ffb_scrub;
    bool  clipping;
    
    // Markers
    bool  user_marker; // Did user press "Mark" button?
};
```

#### B. The Logger Class (`AsyncLogger`)

```cpp
class AsyncLogger {
public:
    void Start(std::string filename);
    void Stop();
    
    // Called from FFBThread (400Hz) - Must be lock-free or extremely fast
    void Log(const LogFrame& frame);

private:
    void WorkerThread(); // The background writer

    std::vector<LogFrame> m_buffer_active;
    std::vector<LogFrame> m_buffer_writing;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;
    std::thread m_thread;
};
```

#### C. The Logic (Double Buffering)

1.  **FFB Loop:** Calls `Log(frame)`.
    *   Acquires lock (very brief).
    *   `m_buffer_active.push_back(frame)`.
    *   If `m_buffer_active.size() > 1000` (approx 2.5 seconds of data), notify the worker thread.
2.  **Worker Thread:**
    *   Wakes up.
    *   Acquires lock.
    *   **Swaps** `m_buffer_active` with `m_buffer_writing`. (This is instant).
    *   Releases lock. (FFB thread can keep writing to the new empty active buffer).
    *   Writes `m_buffer_writing` to disk.
    *   Clears `m_buffer_writing`.

### 4. Making it "Informative" (Analysis Features)

To make these logs actually useful for diagnosing the "Zero Load" or "Instability" issues, we should add:

1.  **Session Header:**
    At the top of the CSV, write the **Config Settings** used for that session.
    ```csv
    # LMUFFB Log v1.0
    # Date: 2025-12-12
    # Settings: Gain=0.8, SoP=0.15, LoadMethod=SuspForce, ManualSlip=True
    Time,Steer,Throttle,...
    ```
    *Why:* When you analyze a log 3 days later, you need to know *which* settings caused that oscillation.

2.  **The "Marker" Button:**
    Add a button in the GUI (or a keyboard shortcut like `Spacebar`) that sets a flag in the log.
    *   *Scenario:* You are driving and feel a weird "clunk". You hit Space.
    *   *Analysis:* Open Excel, filter for `Marker == 1`. You instantly find the exact timestamp of the issue.

3.  **Derived Deltas (Optional but helpful):**
    Log `DeltaTime` (dt).
    *   *Why:* To detect if the FFB loop is stuttering or missing frames. If `dt` spikes from 0.0025 to 0.020, you have a performance problem.

### 5. Analysis Workflow (How to use it)

1.  **MegaLogViewer (Recommended):**
    *   It handles large CSVs easily.
    *   You can plot `Calc Grip` vs `Steering Torque` to see if the understeer effect is triggering correctly.
2.  **Excel / Google Sheets:**
    *   Good for short captures (< 30 seconds).
    *   Use Conditional Formatting on the `Raw Load` column to highlight exactly when it drops to 0.

### 6. Implementation Plan

1.  **Phase 1 (Core):** Implement `AsyncLogger` class with double buffering.
2.  **Phase 2 (Integration):** Add `m_logger` to `FFBEngine`. Call `Log()` at the end of `calculate_force`.
3.  **Phase 3 (GUI):** Add "Start/Stop Logging" toggle in the Troubleshooting window.

**Would you like me to generate the full C++ code for this `AsyncLogger` class?**

## Code for the AsyncLogger class

Here is the complete, header-only implementation of the `AsyncLogger` class, designed to be dropped into your `src/` folder.

It implements the **Double-Buffering** pattern to ensure the 400Hz FFB loop is never blocked by disk I/O.

### 1. Create `src/AsyncLogger.h`

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

// The data structure representing one physics tick (400Hz)
struct LogFrame {
    double timestamp;      // Session time
    
    // Inputs
    float steering_torque;
    float throttle;
    float brake;
    
    // Raw Telemetry (The "Truth")
    float raw_load_fl;
    float raw_grip_fl;
    float raw_susp_force_fl;
    float raw_ride_height_fl;
    float raw_lat_vel;
    
    // Calculated Physics (The "Workaround")
    float calc_load_fl;
    float calc_grip_fl;
    float calc_slip_ratio_fl;
    float calc_slip_angle_fl;
    
    // FFB Outputs (The Result)
    float ffb_total;
    float ffb_sop;
    float ffb_road;
    float ffb_scrub;
    bool  clipping;
    
    // Diagnostics
    bool  marker; // User pressed "Mark"
};

class AsyncLogger {
public:
    static AsyncLogger& Get() {
        static AsyncLogger instance;
        return instance;
    }

    // Start logging to a new file
    void Start(const std::string& filename_prefix = "lmuffb_log") {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_running) return;

        // Generate filename with timestamp: lmuffb_log_2025-12-12_14-30-00.csv
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << filename_prefix << "_" << std::put_time(std::localtime(&time_t), "%Y-%m-%d_%H-%M-%S") << ".csv";
        m_filename = ss.str();

        // Open file and write header
        m_file.open(m_filename);
        if (m_file.is_open()) {
            WriteHeader();
            m_running = true;
            m_worker = std::thread(&AsyncLogger::WorkerThread, this);
            std::cout << "[Logger] Started logging to " << m_filename << std::endl;
        } else {
            std::cerr << "[Logger] Failed to open file: " << m_filename << std::endl;
        }
    }

    // Stop logging and flush remaining data
    void Stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running) return;
            m_running = false;
        }
        m_cv.notify_one(); // Wake worker to finish
        if (m_worker.joinable()) {
            m_worker.join();
        }
        if (m_file.is_open()) {
            m_file.close();
        }
        std::cout << "[Logger] Stopped." << std::endl;
    }

    // FAST: Called from FFB Thread (400Hz)
    // Pushes data to memory buffer. Minimal locking.
    void Log(const LogFrame& frame) {
        if (!m_running) return;

        bool notify = false;
        {
            // Quick lock just to push to vector
            std::lock_guard<std::mutex> lock(m_mutex);
            m_active_buffer.push_back(frame);
            
            // If buffer gets big enough (e.g. 0.5 seconds of data), wake the writer
            if (m_active_buffer.size() >= 200) {
                notify = true;
            }
        }

        if (notify) {
            m_cv.notify_one();
        }
    }

    bool IsLogging() const { return m_running; }

private:
    AsyncLogger() : m_running(false) {
        // Reserve memory to prevent allocations during runtime
        m_active_buffer.reserve(2000);
        m_write_buffer.reserve(2000);
    }
    
    ~AsyncLogger() { Stop(); }

    // No copy
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    void WriteHeader() {
        m_file << "Time,SteerTorque,Throttle,Brake,"
               << "RawLoadFL,RawGripFL,RawSuspForceFL,RawRideHeightFL,RawLatVel,"
               << "CalcLoadFL,CalcGripFL,CalcSlipRatioFL,CalcSlipAngleFL,"
               << "FFB_Total,FFB_SoP,FFB_Road,FFB_Scrub,Clipping,Marker\n";
    }

    // The Background Worker
    void WorkerThread() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                
                // Wait until buffer has data OR we are stopping
                m_cv.wait(lock, [this] { 
                    return !m_active_buffer.empty() || !m_running; 
                });

                // Swap buffers (Fast!)
                // m_active_buffer becomes empty (ready for FFB thread)
                // m_write_buffer gets the data
                m_active_buffer.swap(m_write_buffer);
                
                // If stopped and nothing left to write, exit
                if (!m_running && m_write_buffer.empty()) {
                    break;
                }
            } // Lock released here. FFB thread can continue writing to m_active_buffer.

            // Write to disk (Slow operation, done without holding lock)
            if (!m_write_buffer.empty()) {
                for (const auto& frame : m_write_buffer) {
                    m_file << std::fixed << std::setprecision(4)
                           << frame.timestamp << ","
                           << frame.steering_torque << ","
                           << frame.throttle << ","
                           << frame.brake << ","
                           
                           << frame.raw_load_fl << ","
                           << frame.raw_grip_fl << ","
                           << frame.raw_susp_force_fl << ","
                           << frame.raw_ride_height_fl << ","
                           << frame.raw_lat_vel << ","
                           
                           << frame.calc_load_fl << ","
                           << frame.calc_grip_fl << ","
                           << frame.calc_slip_ratio_fl << ","
                           << frame.calc_slip_angle_fl << ","
                           
                           << frame.ffb_total << ","
                           << frame.ffb_sop << ","
                           << frame.ffb_road << ","
                           << frame.ffb_scrub << ","
                           << (frame.clipping ? "1" : "0") << ","
                           << (frame.marker ? "1" : "0") << "\n";
                }
                m_write_buffer.clear();
            }
        }
    }

    std::ofstream m_file;
    std::string m_filename;
    std::thread m_worker;
    
    std::vector<LogFrame> m_active_buffer; // FFB writes here
    std::vector<LogFrame> m_write_buffer;  // Worker reads here
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;
};

#endif // ASYNCLOGGER_H
```

### 2. Integration Steps

**A. Update `FFBEngine.h`**
Include the logger and populate the frame at the end of `calculate_force`.

```cpp
#include "src/AsyncLogger.h"

// ... inside calculate_force ...

// [After all calculations are done]
if (AsyncLogger::Get().IsLogging()) {
    LogFrame frame;
    frame.timestamp = data->mElapsedTime;
    frame.steering_torque = (float)game_force;
    frame.throttle = (float)data->mUnfilteredThrottle;
    frame.brake = (float)data->mUnfilteredBrake;
    
    // Raw
    frame.raw_load_fl = (float)fl.mTireLoad; // Before fallback!
    frame.raw_grip_fl = (float)fl.mGripFract;
    frame.raw_susp_force_fl = (float)fl.mSuspForce;
    frame.raw_ride_height_fl = (float)fl.mRideHeight;
    frame.raw_lat_vel = (float)fl.mLateralPatchVel;
    
    // Calculated
    frame.calc_load_fl = (float)approximate_load(fl);
    frame.calc_grip_fl = (float)front_grip_res.value;
    frame.calc_slip_ratio_fl = (float)get_slip_ratio(fl);
    frame.calc_slip_angle_fl = (float)m_grip_diag.front_slip_angle;
    
    // Outputs
    frame.ffb_total = (float)norm_force;
    frame.ffb_sop = (float)sop_total;
    frame.ffb_road = (float)road_noise;
    frame.ffb_scrub = (float)drag_force; // If you have this variable
    frame.clipping = (std::abs(norm_force) > 0.99);
    
    // Marker (You need to pass this in or read a global atomic)
    frame.marker = false; 

    AsyncLogger::Get().Log(frame);
}
```

**B. Update `GuiLayer.cpp`**
Add the button to the Troubleshooting window.

```cpp
// Inside DrawDebugWindow or TuningWindow
if (AsyncLogger::Get().IsLogging()) {
    if (ImGui::Button("STOP LOGGING", ImVec2(150, 30))) {
        AsyncLogger::Get().Stop();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1,0,0,1), "RECORDING...");
} else {
    if (ImGui::Button("Start Logging", ImVec2(150, 30))) {
        AsyncLogger::Get().Start();
    }
}
```


