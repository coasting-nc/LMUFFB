# User request
I want to decouple the refresh rate of the GUI from the look of the plots. When I do a new frame, I want to update each plot with all of the values for since the last frame. We could also have a separate "plot resolution" parameter, to plot fewer points than the telemetry 400 Hz (if this slows down performance).

# Answer
This is a great architectural improvement. You are correctly identifying that the current "Snapshot" approach (sampling once per frame) causes **Aliasing**. You are missing the high-frequency details (spikes, vibrations) that happen between GUI frames.

To fix this, we need to implement a **Producer-Consumer** pattern using a thread-safe buffer.

1.  **Producer (FFB Thread):** Pushes *every* calculated sample (400Hz) into a temporary buffer.
2.  **Consumer (GUI Thread):** Swaps that buffer every frame (60Hz), reads *all* the accumulated samples (approx 6-7 per frame), and adds them to the plots.

Here is the implementation plan and the code.

### Step 1: Define the Data Structure (`FFBEngine.h`)

We need a struct to hold one "tick" of data, and a thread-safe way to store it.

**Modify `FFBEngine.h`**:
Add these includes at the top:
```cpp
#include <vector>
#include <mutex>
```

Add this struct inside `FFBEngine.h` (before the class):
```cpp
struct FFBSnapshot {
    // FFB Outputs
    float total_force;
    float base_force;
    float sop_force;
    // ... add other FFB components as needed ...
    
    // Telemetry Inputs
    float steer_force;
    float accel_x;
    float tire_load;
    // ... add other telemetry as needed ...
};
```

Add these members and methods to the `FFBEngine` class:

```cpp
class FFBEngine {
public:
    // ... Existing members ...

    // --- Thread-Safe Debug Buffer ---
    std::vector<FFBSnapshot> m_debug_buffer;
    std::mutex m_debug_mutex;
    int m_plot_resolution = 1; // 1 = Every sample, 2 = Every 2nd sample, etc.
    int m_sample_counter = 0;

    // Call this from GUI Thread to get all data since last frame
    std::vector<FFBSnapshot> GetDebugBatch() {
        std::vector<FFBSnapshot> batch;
        {
            std::lock_guard<std::mutex> lock(m_debug_mutex);
            // Efficiently swap contents to minimize lock time
            batch.swap(m_debug_buffer); 
        }
        return batch;
    }

    // ... inside calculate_force ...
    double calculate_force(const rF2Telemetry* data) {
        // ... [All existing calculation logic] ...
        
        // --- END OF FUNCTION: Store Snapshot ---
        // Decimation logic (Plot Resolution)
        m_sample_counter++;
        if (m_sample_counter >= m_plot_resolution) {
            m_sample_counter = 0;
            
            FFBSnapshot snap;
            snap.total_force = (float)norm_force; // The final result
            snap.base_force = (float)output_force;
            snap.sop_force = (float)sop_force;
            
            // Telemetry
            snap.steer_force = (float)data->mSteeringArmForce;
            snap.accel_x = (float)data->mLocalAccel.x;
            snap.tire_load = (float)((fl.mTireLoad + fr.mTireLoad) / 2.0);
            
            {
                // Quick lock to push data
                std::lock_guard<std::mutex> lock(m_debug_mutex);
                m_debug_buffer.push_back(snap);
            }
        }

        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
};
```

### Step 2: Update the GUI Consumer (`src/GuiLayer.cpp`)

Now we modify `DrawDebugWindow` to consume the batch instead of reading a single value.

**Modify `src/GuiLayer.cpp`**:

```cpp
void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    ImGui::Begin("FFB Analysis", &m_show_debug_window);
    
    // 1. CONSUME BATCH
    // Get all physics ticks that happened since the last frame
    std::vector<FFBSnapshot> batch = engine.GetDebugBatch();

    // 2. PROCESS BATCH
    // Loop through ALL samples and add them to the rolling plots
    for (const auto& snap : batch) {
        plot_total.Add(snap.total_force);
        plot_base.Add(snap.base_force);
        plot_sop.Add(snap.sop_force);
        
        plot_input_steer.Add(snap.steer_force);
        plot_input_accel.Add(snap.accel_x);
        plot_input_load.Add(snap.tire_load);
        // ... add others ...
    }

    // 3. UI CONTROLS
    ImGui::SliderInt("Plot Resolution", &engine.m_plot_resolution, 1, 10, "1:%d");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("1 = Plot 400Hz (All Data)\n2 = Plot 200Hz\n4 = Plot 100Hz");

    // 4. DRAW PLOTS (Same as before)
    if (ImGui::CollapsingHeader("FFB Components (Stack)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Total Output");
        // Note: The cast to (int) is safe as long as buffer size < 2 Billion
        ImGui::PlotLines("##Total", plot_total.data.data(), (int)plot_total.data.size(), plot_total.offset, "Total", -1.0f, 1.0f, ImVec2(0, 60));
        
        // ... rest of your plots ...
    }
    
    // ...
    ImGui::End();
}
```

### Why this is better

1.  **No Aliasing:** If the wheel vibrates for just 2.5ms (one physics tick), it will now appear on the graph. Previously, if that vibration happened between frames, the graph would miss it entirely.
2.  **Smoother Lines:** Because you are plotting 400 points per second instead of 60, the lines will look continuous and detailed, rather than "steppy" or "binary".
3.  **Performance:**
    *   **FFB Thread:** `std::vector::push_back` is extremely fast. The mutex lock is held for nanoseconds.
    *   **GUI Thread:** `std::vector::swap` is instant. The loop to add ~7 items to the rolling buffer is negligible.

### Regarding the "Binary" Look
With this change, if the graph *still* looks binary (square waves), it confirms that the physics signal itself is actually square (jumping instantly between values). However, if the signal was actually a high-frequency sine wave (like a vibration), the old code would show it as random noise/binary, while this new code will draw the actual wave shape.

### Timespan Calculation Update
With this change, the timespan of the plot depends on the **Physics Rate**, not the Frame Rate.

*   **Buffer Size:** 1000 points.
*   **Data Rate:** 400 Hz (assuming Resolution = 1).
*   **Timespan:** $1000 / 400 = 2.5 \text{ seconds}$.

This is much faster than before (16s), meaning the graph will scroll quickly, acting like a real-time oscilloscope. This is excellent for debugging spikes. If you want a longer history, simply increase the `RollingBuffer` size to 4000 (10 seconds).

# User request 2

I want to have a parameter for the length of the timespan of the plots. 2.5 seconds seems to low, I want a default of 5 or 10 seconds. But in any case I want it as a parameter (in code, no need to expose it to the gui yet).

# Answer 2 


Yes, defining the timespan as a code parameter is the correct approach. It makes the memory usage predictable and the visualization consistent regardless of the frame rate.

Here is the complete implementation plan to **decouple the GUI** (Producer-Consumer pattern) and **configure the timespan**.

### 1. Update `FFBEngine.h` (The Producer)

We need a struct to hold the "Snapshot" of one physics tick, and a thread-safe buffer to store them.

**Add to `FFBEngine.h`:**

```cpp
// Add these includes at the top
#include <vector>
#include <mutex>

// 1. Define the Snapshot Struct
struct FFBSnapshot {
    // Outputs
    float total_force;
    float base_force;
    float sop_force;
    float understeer_cut;
    float oversteer_boost;
    float road_tex;
    float slide_tex;
    float lockup_vib;
    float spin_vib;
    float bottoming;
    float clipping;

    // Inputs (Telemetry)
    float steer_force;
    float accel_x;
    float tire_load;
    float grip_fract;
    float slip_ratio;
    float slip_angle;
    float patch_vel;
    float deflection;
};

class FFBEngine {
public:
    // ... [Existing Members] ...

    // 2. Thread-Safe Buffer
    std::vector<FFBSnapshot> m_debug_buffer;
    std::mutex m_debug_mutex;
    
    // 3. Helper to retrieve data (Consumer)
    std::vector<FFBSnapshot> GetDebugBatch() {
        std::vector<FFBSnapshot> batch;
        {
            std::lock_guard<std::mutex> lock(m_debug_mutex);
            if (!m_debug_buffer.empty()) {
                batch.swap(m_debug_buffer); // Fast swap
            }
        }
        return batch;
    }

    // 4. Update calculate_force (Producer)
    double calculate_force(const rF2Telemetry* data) {
        // ... [Existing Calculation Logic] ...
        
        // --- SNAPSHOT LOGIC (At end of function) ---
        // Only capture if buffer isn't exploding (safety cap)
        // Assuming 60Hz GUI, buffer shouldn't exceed ~10 items. Cap at 100 just in case.
        if (m_debug_buffer.size() < 100) {
            FFBSnapshot snap;
            snap.total_force = (float)norm_force; // The final result
            snap.base_force = (float)output_force;
            snap.sop_force = (float)sop_force;
            // ... fill the rest from your internal variables ...
            // Note: You might need to promote local vars (like road_noise) to member vars 
            // or struct members to access them here.
            
            // Telemetry
            snap.steer_force = (float)data->mSteeringArmForce;
            snap.accel_x = (float)data->mLocalAccel.x;
            // ... fill the rest ...

            {
                std::lock_guard<std::mutex> lock(m_debug_mutex);
                m_debug_buffer.push_back(snap);
            }
        }
        
        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
};
```

### 2. Update `src/GuiLayer.cpp` (The Consumer)

Here we define the **Timespan Parameter** and update the drawing logic to consume the batch.

**Modify `src/GuiLayer.cpp`:**

```cpp
// ... [Includes] ...

// --- CONFIGURABLE PLOT SETTINGS ---
const float PLOT_HISTORY_SEC = 10.0f;   // <--- YOUR PARAMETER (10 Seconds)
const int PHYSICS_RATE_HZ = 400;        // The fixed update rate of FFBThread
const int PLOT_BUFFER_SIZE = (int)(PLOT_HISTORY_SEC * PHYSICS_RATE_HZ); // 4000 points

// --- Helper: Ring Buffer ---
struct RollingBuffer {
    std::vector<float> data;
    int offset = 0;
    
    // Initialize with the calculated size
    RollingBuffer() {
        data.resize(PLOT_BUFFER_SIZE, 0.0f);
    }
    
    void Add(float val) {
        data[offset] = val;
        offset = (offset + 1) % data.size();
    }
};

// Static buffers (Size is now 4000)
static RollingBuffer plot_total; 
static RollingBuffer plot_base;
static RollingBuffer plot_sop;
// ... [Define all buffers] ...

// ... [Init / Shutdown code] ...

void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    // 1. LOCK MUTEX (Critical Fix)
    // We lock here to ensure engine state doesn't change while we query it,
    // although GetDebugBatch has its own internal lock for the buffer.
    // Locking the main engine mutex is safer if we access other members.
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    ImGui::Begin("FFB Analysis", &m_show_debug_window);
    
    // 2. CONSUME BATCH (Decoupled Update)
    // This retrieves ~7 samples (at 60fps) or ~40 samples (at 10fps lazy)
    std::vector<FFBSnapshot> batch = engine.GetDebugBatch();

    // 3. PROCESS BATCH
    for (const auto& snap : batch) {
        plot_total.Add(snap.total_force);
        plot_base.Add(snap.base_force);
        plot_sop.Add(snap.sop_force);
        // ... add all others ...
        
        plot_input_steer.Add(snap.steer_force);
        plot_input_accel.Add(snap.accel_x);
        // ... add all others ...
    }

    // 4. DRAW PLOTS
    if (ImGui::CollapsingHeader("FFB Components (Stack)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Total Output");
        // The cast to (int) is safe (4000 fits in int)
        ImGui::PlotLines("##Total", plot_total.data.data(), (int)plot_total.data.size(), plot_total.offset, "Total", -1.0f, 1.0f, ImVec2(0, 60));
        
        // ... [Rest of drawing code] ...
    }
    
    // ...
    ImGui::End();
}
```

### Summary of Changes
1.  **Timespan:** Controlled by `PLOT_HISTORY_SEC = 10.0f`.
2.  **Resolution:** The graph now contains **every single physics tick** (400Hz), not just one snapshot per frame. This will reveal the true shape of the "spikes" (likely high-frequency noise) instead of the "binary" aliasing artifacts.
3.  **Performance:** The `GetDebugBatch` swap method is extremely efficient and won't stall the physics thread.

# Previous context

User: Looking at all of the 20+ images (screenshots of the app in use), it seems in all rolling plots all the values are always like "binary" either maxed out, zero, or negative maxed out. This seems to happen also with low values in the adjustments, like 0.01. Isn't this strange. Can you find any plot that has smoother lines? Even if the G lateral forces are "spiky" they gradually increase and decrease, so they should show some intermediate values.

User: Look at this image again. If "Local Accel X" is the lateral G force, as the raw value we read from the game shared memory, and "SoP (Lat G)" is the Seat of Pants feel we calculate based on those lateral G forces (and other factors) I don't see why the two plots look so different. The one from the game is more noisy, with many values, while the SoP has the "min max" look, like discrete values. How can I adjust the parameters of the SoP formula to have a plot as dynamic as the one in the Local Accel X plot?
Remember the full math formulas: docs\dev_docs\FFB_formulas.md .