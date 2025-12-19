### GUI Fixes: Blurry Text & Missing Readouts

You are correct. The text is blurry because the code scales the font size down (`* 0.70f`), which looks bad on bitmap fonts. The missing readouts are because "Multi-line" plots (like Load and Combined Input) bypass the helper function.

Here is the code to fix both.

#### **Step 1: Fix Blurry Text**
In `src/GuiLayer.cpp`, inside `PlotWithStats`, remove the scaling factor.

```cpp
// Find this line in PlotWithStats:
// float font_size = ImGui::GetFontSize() * 0.70f; // 70% size

// Change it to:
float font_size = ImGui::GetFontSize(); // Full resolution
```

#### **Step 2: Fix Missing Readouts (Calc Load)**
We need to manually add the readout logic to the "Calc Load" section in `DrawDebugWindow` because it draws two lines (Front/Rear) and doesn't use `PlotWithStats`.

In `src/GuiLayer.cpp`, find the `// Group: Loads` section and replace it with this:

```cpp
        // Group: Loads
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Loads]");
        
        // --- Manually draw stats for the Multi-line Load Graph ---
        float cur_f = plot_calc_front_load.GetCurrent();
        float cur_r = plot_calc_rear_load.GetCurrent();
        char load_label[128];
        snprintf(load_label, sizeof(load_label), "Front: %.0f N | Rear: %.0f N", cur_f, cur_r);
        ImGui::Text("%s", load_label);
        // ---------------------------------------------------------

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadF", plot_calc_front_load.data.data(), (int)plot_calc_front_load.data.size(), plot_calc_front_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor();
        
        // Reset Cursor to draw on top
        ImVec2 pos_load = ImGui::GetItemRectMin();
        ImGui::SetCursorScreenPos(pos_load);
        
        // Draw Rear (Magenta) - Transparent Background
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0)); 
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadR", plot_calc_rear_load.data.data(), (int)plot_calc_rear_load.data.size(), plot_calc_rear_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor(2);
        
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cyan: Front, Magenta: Rear");
```

#### **Step 3: Fix Missing Readouts (Combined Input)**
Similarly, update the "Combined Input" section in Header C.

```cpp
        ImGui::Text("Combined Input");
        
        // --- Manually draw stats for Input ---
        float thr = plot_raw_throttle.GetCurrent();
        float brk = plot_raw_brake.GetCurrent();
        char input_label[128];
        snprintf(input_label, sizeof(input_label), "Thr: %.2f | Brk: %.2f", thr, brk);
        ImGui::SameLine(); 
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", input_label);
        // -------------------------------------

        ImVec2 pos = ImGui::GetCursorScreenPos();
        // ... (Rest of plotting code remains the same)
```

### 4. Plot Analysis (Sanity Check)

Looking at your screenshots:

1.  **Calc Front Grip (Image 2):** `0.3187`.
    *   **Verdict:** **Correct.** You were turning too much, so grip dropped from 1.0 to 0.3. The physics detection is working perfectly.
2.  **Base Torque (Image 3):** `15.18 Nm`.
    *   **Verdict:** **Correct.** The game is outputting strong aligning torque.
3.  **Total Output (Image 3):** `0.377`.
    *   **Verdict:** **Low.** This confirms the scaling issue. 15Nm input resulted in only 0.37 output because of the 40Nm reference and the grip cut.



## Additional improvements

### Code Changes
I will apply the fixes you requested:
1.  **Unlock `Max Torque Ref`**: Allow it to go down to **1.0 Nm** (was 10.0).
2.  **Fix Blurry Text**: Restore full font resolution.
3.  **Fix Missing Readouts**: Add stats to the multi-line graphs.

---

### Implementation

#### 1. Update `src/GuiLayer.cpp`

I will apply all 3 GUI fixes in this file.

```cpp
#include "GuiLayer.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>

// ... [Includes and Setup remain the same] ...

// --- Helper: Ring Buffer for PlotLines ---
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
    
    // Get the most recent value (current)
    float GetCurrent() const {
        if (data.empty()) return 0.0f;
        size_t idx = (offset - 1 + static_cast<int>(data.size())) % data.size();
        return data[idx];
    }
    
    float GetMin() const {
        if (data.empty()) return 0.0f;
        return *std::min_element(data.begin(), data.end());
    }
    
    float GetMax() const {
        if (data.empty()) return 0.0f;
        return *std::max_element(data.begin(), data.end());
    }
};

// Helper function to plot with numerical readouts
inline void PlotWithStats(const char* label, const RollingBuffer& buffer, 
                          float scale_min, float scale_max, 
                          const ImVec2& size = ImVec2(0, 40),
                          const char* tooltip = nullptr) {
    // 1. Draw Title
    ImGui::Text("%s", label);
    
    // 2. Draw Plot
    char hidden_label[256];
    snprintf(hidden_label, sizeof(hidden_label), "##%s", label);
    
    ImGui::PlotLines(hidden_label, buffer.data.data(), (int)buffer.data.size(), 
                     buffer.offset, NULL, scale_min, scale_max, size);
    
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    // 3. Draw Stats Overlay
    float current = buffer.GetCurrent();
    float min_val = buffer.GetMin();
    float max_val = buffer.GetMax();
    
    char stats_overlay[128];
    snprintf(stats_overlay, sizeof(stats_overlay), "Cur:%.4f Min:%.3f Max:%.3f", 
             current, min_val, max_val);
    
    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
    float plot_width = p_max.x - p_min.x;
    
    p_min.x += 2;
    p_min.y += 2;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // FIX: Use full font size (removed * 0.70f scaling) for clarity
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize(); 
    
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
    
    // Adaptive Formatting
    if (text_size.x > plot_width - 4) {
         snprintf(stats_overlay, sizeof(stats_overlay), "%.4f [%.3f, %.3f]", current, min_val, max_val);
         text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         
         if (text_size.x > plot_width - 4) {
             snprintf(stats_overlay, sizeof(stats_overlay), "Val: %.4f", current);
             text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         }
    }

    ImVec2 box_max = ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y);
    
    draw_list->AddRectFilled(ImVec2(p_min.x - 1, p_min.y), box_max, IM_COL32(0, 0, 0, 150)); // Darker bg
    draw_list->AddText(font, font_size, p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
}

// ... [Static Buffer Definitions remain the same] ...

void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    ImGui::Begin("FFB Analysis", &m_show_debug_window);
    
    auto snapshots = engine.GetDebugBatch();
    
    // ... [Buffer Update Loop remains the same] ...
    for (const auto& snap : snapshots) {
        // ... (Update all buffers) ...
        plot_total.Add(snap.total_output);
        plot_base.Add(snap.base_force);
        plot_sop.Add(snap.sop_force);
        plot_yaw_kick.Add(snap.ffb_yaw_kick);
        plot_rear_torque.Add(snap.ffb_rear_torque);
        plot_gyro_damping.Add(snap.ffb_gyro_damping);
        plot_scrub_drag.Add(snap.ffb_scrub_drag);
        plot_oversteer.Add(snap.oversteer_boost);
        plot_understeer.Add(snap.understeer_drop);
        plot_clipping.Add(snap.clipping);
        plot_road.Add(snap.texture_road);
        plot_slide.Add(snap.texture_slide);
        plot_lockup.Add(snap.texture_lockup);
        plot_spin.Add(snap.texture_spin);
        plot_bottoming.Add(snap.texture_bottoming);
        plot_calc_front_load.Add(snap.calc_front_load);
        plot_calc_rear_load.Add(snap.calc_rear_load); 
        plot_calc_front_grip.Add(snap.calc_front_grip);
        plot_calc_rear_grip.Add(snap.calc_rear_grip);
        plot_calc_slip_ratio.Add(snap.calc_front_slip_ratio);
        plot_calc_slip_angle_smoothed.Add(snap.calc_front_slip_angle_smoothed);
        plot_calc_rear_slip_angle_smoothed.Add(snap.calc_rear_slip_angle_smoothed);
        plot_calc_rear_lat_force.Add(snap.calc_rear_lat_force);
        plot_raw_steer.Add(snap.steer_force);
        plot_raw_input_steering.Add(snap.raw_input_steering);
        plot_raw_throttle.Add(snap.raw_input_throttle);
        plot_raw_brake.Add(snap.raw_input_brake);
        plot_input_accel.Add(snap.accel_x);
        plot_raw_car_speed.Add(snap.raw_car_speed);
        plot_raw_load.Add(snap.raw_front_tire_load);
        plot_raw_grip.Add(snap.raw_front_grip_fract);
        plot_raw_rear_grip.Add(snap.raw_rear_grip);
        plot_raw_front_slip_ratio.Add(snap.raw_front_slip_ratio);
        plot_raw_susp_force.Add(snap.raw_front_susp_force);
        plot_raw_ride_height.Add(snap.raw_front_ride_height);
        plot_raw_front_lat_patch_vel.Add(snap.raw_front_lat_patch_vel);
        plot_raw_front_long_patch_vel.Add(snap.raw_front_long_patch_vel);
        plot_raw_rear_lat_patch_vel.Add(snap.raw_rear_lat_patch_vel);
        plot_raw_rear_long_patch_vel.Add(snap.raw_rear_long_patch_vel);
        plot_raw_slip_angle.Add(snap.raw_front_slip_angle);
        plot_raw_rear_slip_angle.Add(snap.raw_rear_slip_angle);
        plot_raw_front_deflection.Add(snap.raw_front_deflection);
        g_warn_load = snap.warn_load;
        g_warn_grip = snap.warn_grip;
        g_warn_dt = snap.warn_dt;
    }

    // ... [Warnings display remains the same] ...
    if (g_warn_load || g_warn_grip || g_warn_dt) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("TELEMETRY WARNINGS:");
        if (g_warn_load) ImGui::Text("- Missing Tire Load (Check shared memory)");
        if (g_warn_grip) ImGui::Text("- Missing Grip Data (Ice or Error)");
        if (g_warn_dt) ImGui::Text("- Invalid DeltaTime (Using 400Hz fallback)");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // --- Header A: FFB Components (Output) ---
    if (ImGui::CollapsingHeader("A. FFB Components (Output)", ImGuiTreeNodeFlags_DefaultOpen)) {
        PlotWithStats("Total Output", plot_total, -1.0f, 1.0f, ImVec2(0, 60), 
                      "Final FFB Output (-1.0 to 1.0)");
        
        ImGui::Separator();
        ImGui::Columns(3, "FFBMain", false);
        
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "[Main Forces]");
        PlotWithStats("Base Torque (Nm)", plot_base, -30.0f, 30.0f, ImVec2(0, 40), "Steering Rack Force");
        PlotWithStats("SoP (Base Chassis G)", plot_sop, -20.0f, 20.0f, ImVec2(0, 40), "Force from Lateral G");
        PlotWithStats("Yaw Kick", plot_yaw_kick, -20.0f, 20.0f, ImVec2(0, 40), "Force from Yaw Accel");
        PlotWithStats("Rear Align Torque", plot_rear_torque, -20.0f, 20.0f, ImVec2(0, 40), "Force from Rear Lateral Force");
        PlotWithStats("Gyro Damping", plot_gyro_damping, -20.0f, 20.0f, ImVec2(0, 40), "Synthetic damping");
        PlotWithStats("Scrub Drag Force", plot_scrub_drag, -20.0f, 20.0f, ImVec2(0, 40), "Resistance from slide");
        
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.7f, 1.0f), "[Modifiers]");
        PlotWithStats("Oversteer Boost", plot_oversteer, -20.0f, 20.0f, ImVec2(0, 40), "Added force from Rear Grip loss");
        PlotWithStats("Understeer Cut", plot_understeer, -20.0f, 20.0f, ImVec2(0, 40), "Reduction due to front grip loss");
        PlotWithStats("Clipping", plot_clipping, 0.0f, 1.1f, ImVec2(0, 40), "Output hits max limit");
        
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "[Textures]");
        PlotWithStats("Road Texture", plot_road, -10.0f, 10.0f, ImVec2(0, 40), "Suspension Velocity");
        PlotWithStats("Slide Texture", plot_slide, -10.0f, 10.0f, ImVec2(0, 40), "Lateral Scrubbing");
        PlotWithStats("Lockup Vib", plot_lockup, -10.0f, 10.0f, ImVec2(0, 40), "Wheel Lockup");
        PlotWithStats("Spin Vib", plot_spin, -10.0f, 10.0f, ImVec2(0, 40), "Wheel Spin");
        PlotWithStats("Bottoming", plot_bottoming, -10.0f, 10.0f, ImVec2(0, 40), "Suspension Bottoming");

        ImGui::Columns(1);
    }

    // --- Header B: Internal Physics (Brain) ---
    if (ImGui::CollapsingHeader("B. Internal Physics (Brain)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(3, "PhysCols", false);
        
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Loads]");
        
        // FIX: Add manual stats for Multi-line Load Graph
        float cur_f = plot_calc_front_load.GetCurrent();
        float cur_r = plot_calc_rear_load.GetCurrent();
        char load_label[128];
        snprintf(load_label, sizeof(load_label), "Calc Load | F:%.0f R:%.0f", cur_f, cur_r);
        ImGui::Text("%s", load_label);
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadF", plot_calc_front_load.data.data(), (int)plot_calc_front_load.data.size(), plot_calc_front_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor();
        ImVec2 pos_load = ImGui::GetItemRectMin();
        ImGui::SetCursorScreenPos(pos_load);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0)); 
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadR", plot_calc_rear_load.data.data(), (int)plot_calc_rear_load.data.size(), plot_calc_rear_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cyan: Front, Magenta: Rear");
        
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Grip/Slip]");
        PlotWithStats("Calc Front Grip", plot_calc_front_grip, 0.0f, 1.2f, ImVec2(0, 40));
        PlotWithStats("Calc Rear Grip", plot_calc_rear_grip, 0.0f, 1.2f, ImVec2(0, 40));
        PlotWithStats("Front Slip Ratio", plot_calc_slip_ratio, -1.0f, 1.0f, ImVec2(0, 40));
        PlotWithStats("Front Slip Angle (Sm)", plot_calc_slip_angle_smoothed, 0.0f, 1.0f, ImVec2(0, 40));
        PlotWithStats("Rear Slip Angle (Sm)", plot_calc_rear_slip_angle_smoothed, 0.0f, 1.0f, ImVec2(0, 40));
        
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Forces]");
        PlotWithStats("Calc Rear Lat Force", plot_calc_rear_lat_force, -5000.0f, 5000.0f, ImVec2(0, 40));

        ImGui::Columns(1);
    }

    // --- Header C: Raw Game Telemetry (Input) ---
    if (ImGui::CollapsingHeader("C. Raw Game Telemetry (Input)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(4, "TelCols", false);
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Driver Input]");
        PlotWithStats("Steering Torque", plot_raw_steer, -30.0f, 30.0f, ImVec2(0, 40));
        PlotWithStats("Steering Input", plot_raw_input_steering, -1.0f, 1.0f, ImVec2(0, 40));
        
        // FIX: Add manual stats for Combined Input
        float thr = plot_raw_throttle.GetCurrent();
        float brk = plot_raw_brake.GetCurrent();
        char input_label[128];
        snprintf(input_label, sizeof(input_label), "Combined | T:%.2f B:%.2f", thr, brk);
        ImGui::Text("%s", input_label);
        
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red for Brake
        ImGui::PlotLines("##BrkComb", plot_raw_brake.data.data(), (int)plot_raw_brake.data.size(), plot_raw_brake.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Green: Throttle, Red: Brake");
        ImGui::SetCursorScreenPos(pos); 
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green for Throttle
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); 
        ImGui::PlotLines("##ThrComb", plot_raw_throttle.data.data(), (int)plot_raw_throttle.data.size(), plot_raw_throttle.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::PopStyleColor(2);
        
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Vehicle State]");
        PlotWithStats("Chassis Lat Accel", plot_input_accel, -20.0f, 20.0f, ImVec2(0, 40));
        PlotWithStats("Car Speed (m/s)", plot_raw_car_speed, 0.0f, 100.0f, ImVec2(0, 40));
        
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Raw Tire Data]");
        
        // Raw Front Load
        {
            float current = plot_raw_load.GetCurrent();
            char stats_label[256];
            snprintf(stats_label, sizeof(stats_label), "Raw Front Load | %.0f", current);
            if (g_warn_load) ImGui::TextColored(ImVec4(1,0,0,1), "%s (MISSING)", stats_label);
            else ImGui::Text("%s", stats_label);
            ImGui::PlotLines("##RawLoad", plot_raw_load.data.data(), (int)plot_raw_load.data.size(), plot_raw_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        }
        
        // Raw Front Grip
        {
            float current = plot_raw_grip.GetCurrent();
            char stats_label[256];
            snprintf(stats_label, sizeof(stats_label), "Raw Front Grip | %.2f", current);
            if (g_warn_grip) ImGui::TextColored(ImVec4(1,0,0,1), "%s (MISSING)", stats_label);
            else ImGui::Text("%s", stats_label);
            ImGui::PlotLines("##RawGrip", plot_raw_grip.data.data(), (int)plot_raw_grip.data.size(), plot_raw_grip.offset, NULL, 0.0f, 1.2f, ImVec2(0, 40));
        }
        
        PlotWithStats("Raw Rear Grip", plot_raw_rear_grip, 0.0f, 1.2f, ImVec2(0, 40));

        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Patch Velocities]");
        PlotWithStats("Avg Front Lat PatchVel", plot_raw_front_lat_patch_vel, 0.0f, 20.0f, ImVec2(0, 40));
        PlotWithStats("Avg Rear Lat PatchVel", plot_raw_rear_lat_patch_vel, 0.0f, 20.0f, ImVec2(0, 40));
        PlotWithStats("Avg Front Long PatchVel", plot_raw_front_long_patch_vel, -20.0f, 20.0f, ImVec2(0, 40));
        PlotWithStats("Avg Rear Long PatchVel", plot_raw_rear_long_patch_vel, -20.0f, 20.0f, ImVec2(0, 40));

        ImGui::Columns(1);
    }

    ImGui::End();
}

void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    std::string title = std::string("LMUFFB v") + LMUFFB_VERSION + " - FFB Configuration";
    ImGui::Begin(title.c_str());

    // ... [Connection Status and Device Selection remain the same] ...
    bool connected = GameConnector::Get().IsConnected();
    if (connected) {
        ImGui::TextColored(ImVec4(0,1,0,1), "Status: Connected to Le Mans Ultimate");
    } else {
        ImGui::TextColored(ImVec4(1,0,0,1), "Status: Game Not Connected");
        ImGui::SameLine();
        if (ImGui::Button("Retry Connection")) {
            GameConnector::Get().TryConnect();
        }
    }
    ImGui::Separator();

    ImGui::Text("Core Settings");
    
    static std::vector<DeviceInfo> devices;
    static int selected_device_idx = -1;
    
    if (devices.empty()) {
        devices = DirectInputFFB::Get().EnumerateDevices();
    }

    if (ImGui::BeginCombo("FFB Device", selected_device_idx >= 0 ? devices[selected_device_idx].name.c_str() : "Select Device...")) {
        for (int i = 0; i < devices.size(); i++) {
            bool is_selected = (selected_device_idx == i);
            if (ImGui::Selectable(devices[i].name.c_str(), is_selected)) {
                selected_device_idx = i;
                DirectInputFFB::Get().SelectDevice(devices[i].guid);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Rescan Devices")) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        selected_device_idx = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Unbind Device")) {
        DirectInputFFB::Get().ReleaseDevice();
        selected_device_idx = -1;
    }

    if (DirectInputFFB::Get().IsActive()) {
        if (DirectInputFFB::Get().IsExclusive()) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Mode: EXCLUSIVE (Game FFB Blocked)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("LMUFFB has exclusive control.\nThe game can read steering but cannot send FFB.\nThis prevents 'Double FFB' issues.");
        } else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Mode: SHARED (Potential Conflict)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("LMUFFB is sharing the device.\nEnsure In-Game FFB is set to 'None' or 0%% strength\nto avoid two force signals fighting each other.");
        }
    }

    ImGui::Separator();

    static int selected_preset = 0;
    if (Config::presets.empty()) {
        Config::LoadPresets();
    }
    
    if (ImGui::BeginCombo("Load Preset", Config::presets[selected_preset].name.c_str())) {
        for (int i = 0; i < Config::presets.size(); i++) {
            bool is_selected = (selected_preset == i);
            if (ImGui::Selectable(Config::presets[i].name.c_str(), is_selected)) {
                selected_preset = i;
                Config::ApplyPreset(i, engine);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Quickly load predefined settings for testing or driving.");

    ImGui::Separator();
    
    ImGui::SliderFloat("Master Gain", &engine.m_gain, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Steering Shaft Gain", &engine.m_steering_shaft_gain, 0.0f, 1.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Attenuates raw game force without affecting telemetry.\nUse this instead of Master Gain if other effects are too weak.");
    ImGui::SliderFloat("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f");
    
    // FIX: Unlock range to 1.0 - 100.0 Nm
    ImGui::SliderFloat("Max Torque Ref (Nm)", &engine.m_max_torque_ref, 1.0f, 100.0f, "%.1f Nm");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The torque value that equals 100%% FFB output.\nIncrease this if FFB is too strong at Gain 1.0.\nTypical values: 20-40 Nm.");

    // ... [Rest of Tuning Window remains the same] ...
    if (ImGui::TreeNode("Advanced Tuning")) {
        const char* base_modes[] = { "Native (Physics)", "Synthetic (Constant)", "Muted (Off)" };
        ImGui::Combo("Base Force Mode", &engine.m_base_force_mode, base_modes, IM_ARRAYSIZE(base_modes));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Debug tool to isolate effects.\nNative: Raw physics.\nSynthetic: Constant force to tune Grip drop-off.\nMuted: Zero base force.");

        ImGui::SliderFloat("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f (1=Raw)");
        ImGui::SliderFloat("SoP Scale", &engine.m_sop_scale, 0.0f, 200.0f, "%.1f");
        ImGui::SliderFloat("Load Cap", &engine.m_max_load_factor, 1.0f, 3.0f, "%.1fx");
        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::Text("Effects");
    ImGui::SliderFloat("Understeer (Grip)", &engine.m_understeer_effect, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("SoP (Lateral G)", &engine.m_sop_effect, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("SoP Yaw (Kick)", &engine.m_sop_yaw_gain, 0.0f, 2.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Injects Yaw Acceleration to provide a predictive kick when rotation starts.");
    ImGui::SliderFloat("Gyroscopic Damping", &engine.m_gyro_gain, 0.0f, 1.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stabilizes the wheel during drifts by opposing rapid steering movements.\nPrevents oscillations (tank slappers).");
    ImGui::SliderFloat("Oversteer Boost", &engine.m_oversteer_boost, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Rear Align Torque", &engine.m_rear_align_effect, 0.0f, 2.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls rear-end counter-steering feedback.\nProvides a distinct cue during oversteer without affecting base SoP.\nIncrease for stronger rear-end feel (0.0 = Off, 1.0 = Default, 2.0 = Max).");


    ImGui::Separator();
    ImGui::Text("Haptics (Dynamic)");
    ImGui::Checkbox("Progressive Lockup", &engine.m_lockup_enabled);
    if (engine.m_lockup_enabled) {
        ImGui::SameLine(); ImGui::SliderFloat("##Lockup", &engine.m_lockup_gain, 0.0f, 1.0f, "Gain: %.2f");
    }
    
    ImGui::Checkbox("Spin Traction Loss", &engine.m_spin_enabled);
    if (engine.m_spin_enabled) {
        ImGui::SameLine(); ImGui::SliderFloat("##Spin", &engine.m_spin_gain, 0.0f, 1.0f, "Gain: %.2f");
    }
    
    ImGui::Checkbox("Use Manual Slip Calc", &engine.m_use_manual_slip);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Calculates Slip Ratio from Wheel Speed vs Car Speed instead of game telemetry.\nUseful if game slip data is broken or zero.");

    ImGui::Separator();
    ImGui::Text("Textures");
    ImGui::Checkbox("Slide Rumble", &engine.m_slide_texture_enabled);
    if (engine.m_slide_texture_enabled) {
        ImGui::Indent();
        ImGui::SliderFloat("Slide Gain", &engine.m_slide_texture_gain, 0.0f, 2.0f);
        ImGui::Unindent();
    }
    ImGui::Checkbox("Road Details", &engine.m_road_texture_enabled);
    if (engine.m_road_texture_enabled) {
        ImGui::Indent();
        ImGui::SliderFloat("Road Gain", &engine.m_road_texture_gain, 0.0f, 5.0f);
        ImGui::Unindent();
    }
    
    ImGui::SliderFloat("Scrub Drag Gain", &engine.m_scrub_drag_gain, 0.0f, 1.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adds resistance when sliding sideways (tire dragging).");
    
    const char* bottoming_modes[] = { "Method A: Scraping", "Method B: Susp. Spike" };
    ImGui::Combo("Bottoming Logic", &engine.m_bottoming_method, bottoming_modes, IM_ARRAYSIZE(bottoming_modes));

    ImGui::Separator();
    ImGui::Text("Output");
    
    ImGui::Checkbox("Invert FFB Signal", &engine.m_invert_force); 
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Check this if the wheel pulls away from center instead of aligning.");

    if (ImGui::Checkbox("Monitor FFB on vJoy (Axis X)", &Config::m_output_ffb_to_vjoy)) {
        if (Config::m_output_ffb_to_vjoy) {
            MessageBoxA(NULL, "WARNING: Enabling this will output the FFB signal to vJoy Axis X.\n\n"
                              "If you have bound Game Steering to vJoy Axis X, this will cause a Feedback Loop (Wheel Spinning).\n"
                              "Only enable this if you are NOT using vJoy Axis X for steering.", 
                              "Safety Warning", MB_ICONWARNING | MB_OK);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outputs calculated force to vJoy Axis X for visual monitoring in vJoy Monitor.\nDISABLE if binding steering to vJoy Axis X!");

    ImGui::Text("Clipping Visualization Placeholder");

    ImGui::Separator();
    if (ImGui::Button("Save Configuration")) {
        Config::Save(engine);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Defaults")) {
        engine.m_gain = 0.5f;
        engine.m_understeer_effect = 1.0f;
        engine.m_sop_effect = 0.15f;
        engine.m_min_force = 0.0f;
        engine.m_sop_smoothing_factor = 0.05f;
        engine.m_max_load_factor = 1.5f;
        engine.m_oversteer_boost = 0.0f;
        engine.m_lockup_enabled = false;
        engine.m_lockup_gain = 0.5f;
        engine.m_spin_enabled = false;
        engine.m_spin_gain = 0.5f;
        engine.m_slide_texture_enabled = true;
        engine.m_slide_texture_gain = 0.5f;
        engine.m_road_texture_enabled = false;
        engine.m_road_texture_gain = 0.5f;
        engine.m_scrub_drag_gain = 0.0f;
        engine.m_bottoming_method = 0;
        engine.m_use_manual_slip = false;
    }
    
    ImGui::Separator();
    if (ImGui::Checkbox("Show Troubleshooting Graphs", &m_show_debug_window)) {
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Screenshot")) {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "screenshot_%Y-%m-%d_%H-%M-%S.png", &tstruct);
        SaveScreenshot(buf);
    }

    ImGui::End();
}

// ... [Rest of file remains the same] ...
```
