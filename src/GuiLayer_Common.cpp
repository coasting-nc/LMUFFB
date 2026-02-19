#include "GuiLayer.h"
#include "Version.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include "GuiWidgets.h"
#include "AsyncLogger.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <chrono>
#include <ctime>

#ifdef ENABLE_IMGUI
#include "imgui.h"

static void DisplayRate(const char* label, double rate, double target) {
    ImGui::Text("%s", label);
    ImVec4 color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
    if (rate >= target * 0.95) color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green
    else if (rate >= target * 0.75) color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Yellow
    ImGui::TextColored(color, "%.1f Hz", rate);
}

// External linkage to FFB loop status
extern std::atomic<bool> g_running;
extern std::recursive_mutex g_engine_mutex;

static const float CONFIG_PANEL_WIDTH = 500.0f;
static const int LATENCY_WARNING_THRESHOLD_MS = 15;

// Professional "Flat Dark" Theme
void GuiLayer::SetupGUIStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]       = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]        = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg]        = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);

    colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);

    colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

    ImVec4 accent = ImVec4(0.00f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab]     = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]  = accent;
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.00f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_CheckMark]      = accent;

    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}


static constexpr std::chrono::seconds CONNECT_ATTEMPT_INTERVAL(2);

void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float current_width = Config::show_graphs ? CONFIG_PANEL_WIDTH : viewport->Size.x;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(current_width, viewport->Size.y));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("MainUI", nullptr, flags);

    ImGui::TextColored(ImVec4(1, 1, 1, 0.4f), "lmuFFB v%s", LMUFFB_VERSION);
    ImGui::Separator();

    static std::chrono::steady_clock::time_point last_check_time = std::chrono::steady_clock::now();

    if (!GameConnector::Get().IsConnected()) {
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Connecting to LMU...");
      if (std::chrono::steady_clock::now() - last_check_time > CONNECT_ATTEMPT_INTERVAL) {
        last_check_time = std::chrono::steady_clock::now();
        GameConnector::Get().TryConnect();
      }
    } else {
      ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected to LMU");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "| FFB: %.0fHz | Tel: %.0fHz", engine.m_ffb_rate, engine.m_telemetry_rate);
    }

    static std::vector<DeviceInfo> devices;
    static int selected_device_idx = -1;

    if (devices.empty()) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        if (selected_device_idx == -1 && !Config::m_last_device_guid.empty()) {
            GUID target = DirectInputFFB::StringToGuid(Config::m_last_device_guid);
            for (int i = 0; i < (int)devices.size(); i++) {
                if (memcmp(&devices[i].guid, &target, sizeof(GUID)) == 0) {
                    selected_device_idx = i;
                    DirectInputFFB::Get().SelectDevice(devices[i].guid);
                    break;
                }
            }
        }
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
    if (ImGui::BeginCombo("FFB Device", selected_device_idx >= 0 ? devices[selected_device_idx].name.c_str() : "Select Device...")) {
        for (int i = 0; i < (int)devices.size(); i++) {
            bool is_selected = (selected_device_idx == i);
            ImGui::PushID(i);
            if (ImGui::Selectable(devices[i].name.c_str(), is_selected)) {
                selected_device_idx = i;
                DirectInputFFB::Get().SelectDevice(devices[i].guid);
                Config::m_last_device_guid = DirectInputFFB::GuidToString(devices[i].guid);
                Config::Save(engine);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select the DirectInput device to receive Force Feedback signals.\nTypically your steering wheel.");

    ImGui::SameLine();
    if (ImGui::Button("Rescan")) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        selected_device_idx = -1;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh the list of available DirectInput devices.");
    ImGui::SameLine();
    if (ImGui::Button("Unbind")) {
        DirectInputFFB::Get().ReleaseDevice();
        selected_device_idx = -1;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Release the current device and disable FFB output.");

    if (DirectInputFFB::Get().IsActive()) {
        if (DirectInputFFB::Get().IsExclusive()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Mode: EXCLUSIVE (Game FFB Blocked)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("lmuFFB has exclusive control.\nThe game can read steering but cannot send FFB.\nThis prevents 'Double FFB' issues.");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Mode: SHARED (Potential Conflict)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("lmuFFB is sharing the device.\nEnsure In-Game FFB is disabled\nto avoid LMU reacquiring the device.");
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No device selected.");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Please select your steering wheel from the 'FFB Device' menu above.");
    }

    if (ImGui::Checkbox("Always on Top", &Config::m_always_on_top)) {
        SetWindowAlwaysOnTopPlatform(Config::m_always_on_top);
        Config::Save(engine);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Keep the lmuFFB window visible over other applications (including the game).");
    ImGui::SameLine();

    bool toggled = Config::show_graphs;
    if (ImGui::Checkbox("Graphs", &toggled)) {
        SaveCurrentWindowGeometryPlatform(Config::show_graphs);
        Config::show_graphs = toggled;
        int target_w = Config::show_graphs ? Config::win_w_large : Config::win_w_small;
        int target_h = Config::show_graphs ? Config::win_h_large : Config::win_h_small;
        ResizeWindowPlatform(Config::win_pos_x, Config::win_pos_y, target_w, target_h);
        Config::Save(engine);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show real-time physics and output graphs for debugging.\nIncreases window width.");

    ImGui::Separator();
    bool is_logging = AsyncLogger::Get().IsLogging();
    if (is_logging) {
         if (ImGui::Button("STOP LOG", ImVec2(80, 0))) {
             AsyncLogger::Get().Stop();
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Finish recording and save the log file.");
         ImGui::SameLine();
         float time = (float)ImGui::GetTime();
         bool blink = (fmod(time, 1.0f) < 0.5f);
         ImGui::TextColored(blink ? ImVec4(1,0,0,1) : ImVec4(0.6f,0,0,1), "REC");
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Currently recording high-frequency telemetry data at 100Hz.");

         ImGui::SameLine();
         size_t bytes = AsyncLogger::Get().GetFileSizeBytes();
         if (bytes < 1024 * 1024)
             ImGui::Text("%zu f (%.0f KB)", AsyncLogger::Get().GetFrameCount(), (float)bytes / 1024.0f);
         else
             ImGui::Text("%zu f (%.1f MB)", AsyncLogger::Get().GetFrameCount(), (float)bytes / (1024.0f * 1024.0f));

         ImGui::SameLine();
         if (ImGui::Button("MARKER")) {
             AsyncLogger::Get().SetMarker();
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add a timestamped marker to the log file to tag an interesting event.");
    } else {
         if (ImGui::Button("START LOGGING", ImVec2(120, 0))) {
             SessionInfo info;
             info.app_version = LMUFFB_VERSION;
             if (engine.m_vehicle_name[0] != '\0') info.vehicle_name = engine.m_vehicle_name;
             else info.vehicle_name = "UnknownCar";

             if (engine.m_track_name[0] != '\0') info.track_name = engine.m_track_name;
             else info.track_name = "UnknownTrack";

             info.driver_name = "Auto";

             info.gain = engine.m_gain;
             info.understeer_effect = engine.m_understeer_effect;
             info.sop_effect = engine.m_sop_effect;
             info.slope_enabled = engine.m_slope_detection_enabled;
             info.slope_sensitivity = engine.m_slope_sensitivity;
             info.slope_threshold = (float)engine.m_slope_min_threshold;
             info.slope_alpha_threshold = engine.m_slope_alpha_threshold;
             info.slope_decay_rate = engine.m_slope_decay_rate;
             info.torque_passthrough = engine.m_torque_passthrough;

             AsyncLogger::Get().Start(info, Config::m_log_path);
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("Start recording high-frequency telemetry and FFB data to a CSV file for analysis.");
         ImGui::SameLine();
         ImGui::TextDisabled("(Diagnostics)");
    }


    ImGui::Separator();

    static int selected_preset = 0;

    auto FormatDecoupled = [&](float val, float base_nm) {
        float estimated_nm = val * base_nm;
        static char buf[64];
        snprintf(buf, 64, "%.1f%%%% (~%.1f Nm)", val * 100.0f, estimated_nm);
        return (const char*)buf;
    };

    auto FormatPct = [&](float val) {
        static char buf[32];
        snprintf(buf, 32, "%.1f%%%%", val * 100.0f);
        return (const char*)buf;
    };

    auto FloatSetting = [&](const char* label, float* v, float min, float max, const char* fmt = "%.2f", const char* tooltip = nullptr, std::function<void()> decorator = nullptr) {
        GuiWidgets::Result res = GuiWidgets::Float(label, v, min, max, fmt, tooltip, decorator);
        if (res.deactivated) {
            Config::Save(engine);
        }
    };

    auto BoolSetting = [&](const char* label, bool* v, const char* tooltip = nullptr) {
        GuiWidgets::Result res = GuiWidgets::Checkbox(label, v, tooltip);
        if (res.deactivated) {
            Config::Save(engine);
        }
    };

    auto IntSetting = [&](const char* label, int* v, const char* const items[], int items_count, const char* tooltip = nullptr) {
        GuiWidgets::Result res = GuiWidgets::Combo(label, v, items, items_count, tooltip);
        if (res.changed) {
            std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
            // v is already updated by ImGui, but we lock to ensure visibility and consistency
            Config::Save(engine);
        }
    };

    if (ImGui::TreeNodeEx("Presets and Configuration", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        if (Config::presets.empty()) Config::LoadPresets();

        static bool first_run = true;
        if (first_run && !Config::presets.empty()) {
            for (int i = 0; i < (int)Config::presets.size(); i++) {
                if (Config::presets[i].name == Config::m_last_preset_name) {
                    selected_preset = i;
                    break;
                }
            }
            first_run = false;
        }

        static std::string preview_buf;
        const char* preview_value = "Custom";
        if (selected_preset >= 0 && selected_preset < (int)Config::presets.size()) {
            preview_buf = Config::presets[selected_preset].name;
            if (Config::IsEngineDirtyRelativeToPreset(selected_preset, engine)) {
                preview_buf += "*";
            }
            preview_value = preview_buf.c_str();
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
        if (ImGui::BeginCombo("Load Preset", preview_value)) {
            for (int i = 0; i < (int)Config::presets.size(); i++) {
                bool is_selected = (selected_preset == i);
                ImGui::PushID(i);
                if (ImGui::Selectable(Config::presets[i].name.c_str(), is_selected)) {
                    selected_preset = i;
                    Config::ApplyPreset(i, engine);
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        static char new_preset_name[64] = "";
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
        ImGui::InputText("##NewPresetName", new_preset_name, 64);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enter a name for your new user preset.");
        ImGui::SameLine();
        if (ImGui::Button("Save New")) {
            if (strlen(new_preset_name) > 0) {
                Config::AddUserPreset(std::string(new_preset_name), engine);
                for (int i = 0; i < (int)Config::presets.size(); i++) {
                    if (Config::presets[i].name == std::string(new_preset_name)) {
                        selected_preset = i;
                        break;
                    }
                }
                new_preset_name[0] = '\0';
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create a new user preset from the current settings.");

        if (ImGui::Button("Save Current Config")) {
            if (selected_preset >= 0 && selected_preset < (int)Config::presets.size() && !Config::presets[selected_preset].is_builtin) {
                Config::AddUserPreset(Config::presets[selected_preset].name, engine);
            } else {
                Config::Save(engine);
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save modifications to the selected user preset or global calibration.");
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults")) {
            Config::ApplyPreset(0, engine);
            selected_preset = 0;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Revert all settings to factory default (T300 baseline).");
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) {
            if (selected_preset >= 0) {
                Config::DuplicatePreset(selected_preset, engine);
                for (int i = 0; i < (int)Config::presets.size(); i++) {
                    if (Config::presets[i].name == Config::m_last_preset_name) {
                        selected_preset = i;
                        break;
                    }
                }
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create a copy of the currently selected preset.");
        ImGui::SameLine();
        bool can_delete = (selected_preset >= 0 && selected_preset < (int)Config::presets.size() && !Config::presets[selected_preset].is_builtin);
        if (!can_delete) ImGui::BeginDisabled();
        if (ImGui::Button("Delete")) {
            Config::DeletePreset(selected_preset, engine);
            selected_preset = 0;
            Config::ApplyPreset(0, engine);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove the selected user preset (builtin presets are protected).");
        if (!can_delete) ImGui::EndDisabled();

        ImGui::Separator();
        if (ImGui::Button("Import Preset...")) {
            std::string path;
            if (OpenPresetFileDialogPlatform(path)) {
                if (Config::ImportPreset(path, engine)) {
                    selected_preset = (int)Config::presets.size() - 1;
                }
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Import an external .ini preset file.");
        ImGui::SameLine();
        if (ImGui::Button("Export Selected...")) {
            if (selected_preset >= 0 && selected_preset < (int)Config::presets.size()) {
                std::string path;
                std::string defaultName = Config::presets[selected_preset].name + ".ini";
                if (SavePresetFileDialogPlatform(path, defaultName)) {
                    Config::ExportPreset(selected_preset, path);
                }
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Export the current preset to an external .ini file.");

        ImGui::TreePop();
    }

    ImGui::Spacing();

    ImGui::Columns(2, "SettingsGrid", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);

    if (ImGui::TreeNodeEx("General FFB", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Spacing();
        bool use_in_game_ffb = (engine.m_torque_source == 1);
        if (GuiWidgets::Checkbox("Use In-Game FFB (400Hz Native)", &use_in_game_ffb,
                    "Recommended for LMU 1.2+. Uses the high-frequency FFB channel directly from the game.\n"
                    "Matches the game's internal physics rate for maximum fidelity.").changed) {
            std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
            engine.m_torque_source = use_in_game_ffb ? 1 : 0;
            Config::Save(engine);
        }

        BoolSetting("Invert FFB Signal", &engine.m_invert_force, "Check this if the wheel pulls away from center instead of aligning.");
        FloatSetting("Master Gain", &engine.m_gain, 0.0f, 2.0f, FormatPct(engine.m_gain), "Global scale factor for all forces.\n100% = No attenuation.\nReduce if experiencing heavy clipping.");
        FloatSetting("Wheelbase Max Torque", &engine.m_wheelbase_max_nm, 1.0f, 50.0f, "%.1f Nm", "The absolute maximum physical torque your wheelbase can produce (e.g., 15.0 for Simagic Alpha, 4.0 for T300).");
        FloatSetting("Target Rim Torque", &engine.m_target_rim_nm, 1.0f, 50.0f, "%.1f Nm", "The maximum force you want to feel in your hands during heavy cornering.");
        FloatSetting("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f", "Boosts small forces to overcome mechanical friction/deadzone.");

        if (ImGui::TreeNodeEx("Soft Lock", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::NextColumn(); ImGui::NextColumn();
            BoolSetting("Enable Soft Lock", &engine.m_soft_lock_enabled, "Provides resistance when the steering wheel reaches the car's maximum steering range.");
            if (engine.m_soft_lock_enabled) {
                FloatSetting("  Stiffness", &engine.m_soft_lock_stiffness, 0.0f, 100.0f, "%.1f", "Intensity of the spring force pushing back from the limit.");
                FloatSetting("  Damping", &engine.m_soft_lock_damping, 0.0f, 5.0f, "%.2f", "Prevents bouncing and oscillation at the steering limit.");
            }
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::TreeNodeEx("Front Axle (Understeer)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("Steering Shaft Gain", &engine.m_steering_shaft_gain, 0.0f, 2.0f, FormatPct(engine.m_steering_shaft_gain), "Scales the raw steering torque from the physics engine.");

        FloatSetting("Steering Shaft Smoothing", &engine.m_steering_shaft_smoothing, 0.000f, 0.100f, "%.3f s",
            "Low Pass Filter applied ONLY to the raw game force.",
            [&]() {
                int ms = (int)(engine.m_steering_shaft_smoothing * 1000.0f + 0.5f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 2.0f, FormatPct(engine.m_understeer_effect),
            "Scales how much front grip loss reduces steering force.");

        FloatSetting("Dynamic Weight", &engine.m_dynamic_weight_gain, 0.0f, 2.0f, FormatPct(engine.m_dynamic_weight_gain),
            "Scales steering weight based on longitudinal load transfer.\n"
            "Heavier under braking, lighter under acceleration.");

        FloatSetting("  Weight Smoothing", &engine.m_dynamic_weight_smoothing, 0.000f, 0.500f, "%.3f s",
            "Filters the Dynamic Weight signal to simulate suspension damping.\n"
            "Higher = Smoother weight transfer feel, but less instant.\n"
            "Recommended: 0.100s - 0.200s.");

        const char* base_modes[] = { "Native (Steering Shaft Torque)", "Synthetic (Constant)", "Muted (Off)" };
        IntSetting("Base Force Mode", &engine.m_base_force_mode, base_modes, sizeof(base_modes)/sizeof(base_modes[0]),
            "Debug tool to isolate effects.\nNative: Normal Operation.\nSynthetic: Constant force to test direction.\nMuted: Disables base physics (good for tuning vibrations).");

        const char* torque_sources[] = { "Shaft Torque (100Hz Legacy)", "In-Game FFB (400Hz LMU 1.2+)" };
        IntSetting("Torque Source", &engine.m_torque_source, torque_sources, sizeof(torque_sources)/sizeof(torque_sources[0]),
            "Select the telemetry channel for base steering torque.\n"
            "Shaft Torque: Standard rF2 physics channel (typically 100Hz).\n"
            "In-Game FFB: New LMU high-frequency channel (native 400Hz). RECOMMENDED.\n"
            "This is the actual FFB signal processed by the game engine.");

        BoolSetting("Pure Passthrough", &engine.m_torque_passthrough,
            "Bypasses LMUFFB's internal Understeer and Dynamic Weight modulation for the base steering torque.\n"
            "Recommended when using In-Game FFB (400Hz) if you prefer the game's native FFB modulation.");

        if (ImGui::TreeNodeEx("Signal Filtering", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::NextColumn(); ImGui::NextColumn();

            BoolSetting("  Flatspot Suppression", &engine.m_flatspot_suppression, "Dynamic Notch Filter that targets wheel rotation frequency.\nSuppresses vibrations caused by tire flatspots.");
            if (engine.m_flatspot_suppression) {
                FloatSetting("    Filter Width (Q)", &engine.m_notch_q, 0.5f, 10.0f, "Q: %.2f", "Quality Factor of the Notch Filter.\nHigher = Narrower bandwidth (surgical removal).\nLower = Wider bandwidth (affects surrounding frequencies).");
                FloatSetting("    Suppression Strength", &engine.m_flatspot_strength, 0.0f, 1.0f, "%.2f", "How strongly to mute the flatspot vibration.\n1.0 = 100% removal.");
                ImGui::Text("    Est. / Theory Freq");
                ImGui::NextColumn();
                ImGui::TextDisabled("%.1f Hz / %.1f Hz", engine.m_debug_freq, engine.m_theoretical_freq);
                ImGui::NextColumn();
            }

            BoolSetting("  Static Noise Filter", &engine.m_static_notch_enabled, "Fixed frequency notch filter to remove hardware resonance or specific noise.");
            if (engine.m_static_notch_enabled) {
                FloatSetting("    Target Frequency", &engine.m_static_notch_freq, 10.0f, 100.0f, "%.1f Hz", "Center frequency to suppress.");
                FloatSetting("    Filter Width", &engine.m_static_notch_width, 0.1f, 10.0f, "%.1f Hz", "Bandwidth of the notch filter.\nLarger = Blocks more frequencies around the target.");
            }

            ImGui::TreePop();
        } else {
            ImGui::NextColumn(); ImGui::NextColumn();
        }

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::TreeNodeEx("Rear Axle (Oversteer)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("Lateral G Boost (Slide)", &engine.m_oversteer_boost, 0.0f, 4.0f, FormatPct(engine.m_oversteer_boost),
            "Increases the Lateral G (SoP) force when the rear tires lose grip.\nMakes the car feel heavier during a slide, helping you judge the momentum.\nShould build up slightly more gradually than Rear Align Torque,\nreflecting the inertia of the car's mass swinging out.\nIt's a sustained force that tells you about the magnitude of the slide\nTuning Goal: The driver should feel the direction of the counter-steer (Rear Align)\nand the effort required to hold it (Lateral G Boost).");
        FloatSetting("Lateral G", &engine.m_sop_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_sop_effect, FFBEngine::BASE_NM_SOP_LATERAL), "Represents Chassis Roll, simulates the weight of the car leaning in the corner.");
        FloatSetting("SoP Self-Aligning Torque", &engine.m_rear_align_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_rear_align_effect, FFBEngine::BASE_NM_REAR_ALIGN),
            "Counter-steering force generated by rear tire slip.\nShould build up very quickly after the Yaw Kick, as the slip angle develops.\nThis is the active \"pull.\"\nTuning Goal: The driver should feel the direction of the counter-steer (Rear Align)\nand the effort required to hold it (Lateral G Boost).");
        FloatSetting("Yaw Kick", &engine.m_sop_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_sop_yaw_gain, FFBEngine::BASE_NM_YAW_KICK),
            "This is the earliest cue for rear stepping out. It's a sharp, momentary impulse that signals the onset of rotation.\nBased on Yaw Acceleration.");
        FloatSetting("  Activation Threshold", &engine.m_yaw_kick_threshold, 0.0f, 10.0f, "%.2f rad/s²", "Minimum yaw acceleration required to trigger the kick.\nIncrease to filter out road noise and small vibrations.");

        FloatSetting("  Kick Response", &engine.m_yaw_accel_smoothing, 0.000f, 0.050f, "%.3f s",
            "Low Pass Filter for the Yaw Kick signal.\nSmoothes out kick noise.\nLower = Sharper/Faster kick.\nHigher = Duller/Softer kick.",
            [&]() {
                int ms = (int)(engine.m_yaw_accel_smoothing * 1000.0f + 0.5f);
                ImVec4 color = (ms <= 15) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms", ms);
            });

        FloatSetting("Gyro Damping", &engine.m_gyro_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_gyro_gain, FFBEngine::BASE_NM_GYRO_DAMPING), "Simulates the gyroscopic solidity of the spinning wheels.\nResists rapid steering movements.\nPrevents oscillation and 'Tank Slappers'.\nActs like a steering damper.");

        FloatSetting("  Gyro Smooth", &engine.m_gyro_smoothing, 0.000f, 0.050f, "%.3f s",
            "Filters the steering velocity signal used for damping.\nReduces noise in the damping effect.\nLow = Crisper damping, High = Smoother.",
            [&]() {
                int ms = (int)(engine.m_gyro_smoothing * 1000.0f + 0.5f);
                ImVec4 color = (ms <= 20) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms", ms);
            });

        ImGui::TextColored(ImVec4(0.0f, 0.6f, 0.85f, 1.0f), "Advanced SoP");
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f",
            "Filters the Lateral G signal.\nReduces jerkiness in the SoP effect.",
            [&]() {
                int ms = (int)((1.0f - engine.m_sop_smoothing_factor) * 100.0f + 0.5f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        FloatSetting("Grip Smoothing", &engine.m_grip_smoothing_steady, 0.000f, 0.100f, "%.3f s",
            "Filters the final estimated grip value.\n"
            "Uses an adaptive non-linear filter: smooths steady-state noise\n"
            "but maintains zero-latency during rapid grip loss events.\n"
            "Recommended: 0.030s - 0.060s.");

        FloatSetting("  SoP Scale", &engine.m_sop_scale, 0.0f, 20.0f, "%.2f", "Multiplies the raw G-force signal before limiting.\nAdjusts the dynamic range of the SoP effect.");

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::TreeNodeEx("Grip & Slip Angle Estimation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("Slip Angle Smoothing", &engine.m_slip_angle_smoothing, 0.000f, 0.100f, "%.3f s",
            "Applies a time-based filter (LPF) to the Calculated Slip Angle used to estimate tire grip.\n"
            "Smooths the high fluctuations from lateral and longitudinal velocity,\nespecially over bumps or curbs.\n"
            "Affects: Understeer effect, Rear Aligning Torque.",
            [&]() {
                int ms = (int)(engine.m_slip_angle_smoothing * 1000.0f + 0.5f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        FloatSetting("Chassis Inertia (Load)", &engine.m_chassis_inertia_smoothing, 0.000f, 0.100f, "%.3f s",
            "Simulation time for weight transfer.\nSimulates how fast the suspension settles.\nAffects calculated tire load magnitude.\n25ms = Stiff Race Car.\n50ms = Soft Road Car.",
            [&]() {
                int ms = (int)(engine.m_chassis_inertia_smoothing * 1000.0f + 0.5f);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Simulation: %d ms", ms);
            });

        FloatSetting("Optimal Slip Angle", &engine.m_optimal_slip_angle, 0.05f, 0.20f, "%.2f rad",
            "The slip angle THRESHOLD above which grip loss begins.\n"
            "Set this HIGHER than the car's physical peak slip angle.\n"
            "Recommended: 0.10 for LMDh/LMP2, 0.12 for GT3.\n\n"
            "Lower = More sensitive (force drops earlier).\n"
            "Higher = More buffer zone before force drops.\n\n"
            "NOTE: If the wheel feels too light at the limit, INCREASE this value.\n"
            "Affects: Understeer Effect, Lateral G Boost (Slide), Slide Texture.");
        FloatSetting("Optimal Slip Ratio", &engine.m_optimal_slip_ratio, 0.05f, 0.20f, "%.2f",
            "The longitudinal slip ratio (0.0-1.0) where peak braking/traction occurs.\n"
            "Typical: 0.12 - 0.15 (12-15%).\n"
            "Used to estimate grip loss under braking/acceleration.\n"
            "Affects: How much braking/acceleration contributes to calculated grip loss.");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Slope Detection (Experimental)");
        ImGui::NextColumn(); ImGui::NextColumn();

        bool prev_slope_enabled = engine.m_slope_detection_enabled;
        GuiWidgets::Result slope_res = GuiWidgets::Checkbox("Enable Slope Detection", &engine.m_slope_detection_enabled,
            "Replaces static 'Optimal Slip Angle' threshold with dynamic derivative monitoring.\n\n"
            "When enabled:\n"
            "• Grip is estimated by tracking the slope of lateral-G vs slip angle\n"
            "• Automatically adapts to tire temperature, wear, and conditions\n"
            "• 'Optimal Slip Angle' and 'Optimal Slip Ratio' settings are IGNORED\n\n"
            "When disabled:\n"
            "• Uses the static threshold method (default behavior)");

        if (slope_res.changed) {
            if (!prev_slope_enabled && engine.m_slope_detection_enabled) {
                engine.m_slope_buffer_count = 0;
                engine.m_slope_buffer_index = 0;
                engine.m_slope_smoothed_output = 1.0;
            }
        }
        if (slope_res.deactivated) {
            Config::Save(engine);
        }

        if (engine.m_slope_detection_enabled && engine.m_oversteer_boost > 0.01f) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                "Note: Lateral G Boost (Slide) is auto-disabled when Slope Detection is ON.");
            ImGui::NextColumn(); ImGui::NextColumn();
        }

        if (engine.m_slope_detection_enabled) {
            int window = engine.m_slope_sg_window;
            if (ImGui::SliderInt("  Filter Window", &window, 5, 41)) {
                if (window % 2 == 0) window++;
                engine.m_slope_sg_window = window;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Savitzky-Golay filter window size (samples).\n\n"
                    "Larger = Smoother but higher latency\n"
                    "Smaller = Faster response but noisier\n\n"
                    "Recommended:\n"
                    "  Direct Drive: 11-15\n"
                    "  Belt Drive: 15-21\n"
                    "  Gear Drive: 21-31\n\n"
                    "Must be ODD (enforced automatically).");
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

            ImGui::SameLine();
            float latency_ms = (float)(engine.m_slope_sg_window / 2) * 2.5f;
            ImVec4 color = (latency_ms < 25.0f) ? ImVec4(0,1,0,1) : ImVec4(1,0.5f,0,1);
            ImGui::TextColored(color, "~%.0f ms latency", latency_ms);
            ImGui::NextColumn(); ImGui::NextColumn();

            FloatSetting("  Sensitivity", &engine.m_slope_sensitivity, 0.1f, 5.0f, "%.1fx",
                "Multiplier for slope-to-grip conversion.\n"
                "Higher = More aggressive grip loss detection.\n"
                "Lower = Smoother, less pronounced effect.");

            if (ImGui::TreeNode("Advanced Slope Settings")) {
                ImGui::NextColumn(); ImGui::NextColumn();
                FloatSetting("  Slope Threshold", &engine.m_slope_min_threshold, -1.0f, 0.0f, "%.2f", "Slope value below which grip loss begins.\nMore negative = Later detection (safer).");
                FloatSetting("  Output Smoothing", &engine.m_slope_smoothing_tau, 0.005f, 0.100f, "%.3f s", "Time constant for grip factor smoothing.\nPrevents abrupt FFB changes.");

                ImGui::Separator();
                ImGui::Text("Stability Fixes (v0.7.3)");
                ImGui::NextColumn(); ImGui::NextColumn();
                FloatSetting("  Alpha Threshold", &engine.m_slope_alpha_threshold, 0.001f, 0.100f, "%.3f", "Confidence threshold for slope detection.\nHigher = Stricter (less noise, potentially slower).");
                FloatSetting("  Decay Rate", &engine.m_slope_decay_rate, 0.5f, 20.0f, "%.1f", "How quickly the grip factor recovers after a slide.\nHigher = Faster recovery.");
                BoolSetting("  Confidence Gate", &engine.m_slope_confidence_enabled, "Ensures slope changes are statistically significant before applying grip loss.");

                ImGui::TreePop();
            } else {
                ImGui::NextColumn(); ImGui::NextColumn();
            }

            ImGui::Text("  Live Slope: %.3f | Grip: %.0f%%",
                engine.m_slope_current,
                engine.m_slope_smoothed_output * 100.0f);
            ImGui::NextColumn(); ImGui::NextColumn();
        }

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::TreeNodeEx("Braking & Lockup", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        BoolSetting("Lockup Vibration", &engine.m_lockup_enabled, "Simulates tire judder when wheels are locked under braking.");
        if (engine.m_lockup_enabled) {
            FloatSetting("  Lockup Strength", &engine.m_lockup_gain, 0.0f, 3.0f, FormatDecoupled(engine.m_lockup_gain, FFBEngine::BASE_NM_LOCKUP_VIBRATION), "Controls the intensity of the vibration when wheels lock up.\nScales with wheel load and speed.");
            FloatSetting("  Brake Load Cap", &engine.m_brake_load_cap, 1.0f, 10.0f, "%.2fx", "Scales vibration intensity based on tire load.\nPrevents weak vibrations during high-speed heavy braking.");
            FloatSetting("  Vibration Pitch", &engine.m_lockup_freq_scale, 0.5f, 2.0f, "%.2fx", "Scales the frequency of lockup and wheel spin vibrations.\nMatch to your hardware resonance.");

            ImGui::Separator();
            ImGui::Text("Response Curve");
            ImGui::NextColumn(); ImGui::NextColumn();

            FloatSetting("  Gamma", &engine.m_lockup_gamma, 0.1f, 3.0f, "%.1f", "Response Curve Non-Linearity.\n1.0 = Linear.\n>1.0 = Progressive (Starts weak, gets strong fast).\n<1.0 = Aggressive (Starts strong). 2.0=Quadratic, 3.0=Cubic (Late/Sharp)");
            FloatSetting("  Start Slip %", &engine.m_lockup_start_pct, 1.0f, 10.0f, "%.1f%%", "Slip percentage where vibration begins.\n1.0% = Immediate feedback.\n5.0% = Only on deep lock.");
            FloatSetting("  Full Slip %", &engine.m_lockup_full_pct, 5.0f, 25.0f, "%.1f%%", "Slip percentage where vibration reaches maximum intensity.");

            ImGui::Separator();
            ImGui::Text("Prediction (Advanced)");
            ImGui::NextColumn(); ImGui::NextColumn();

            FloatSetting("  Sensitivity", &engine.m_lockup_prediction_sens, 10.0f, 100.0f, "%.0f", "Angular Deceleration Threshold.\nHow aggressively the system predicts a lockup before it physically occurs.\nLower = More sensitive (triggers earlier).\nHigher = Less sensitive.");
            FloatSetting("  Bump Rejection", &engine.m_lockup_bump_reject, 0.1f, 5.0f, "%.1f m/s", "Suspension velocity threshold.\nDisables prediction on bumpy surfaces to prevent false positives.\nIncrease for bumpy tracks (Sebring).");
            FloatSetting("  Rear Boost", &engine.m_lockup_rear_boost, 1.0f, 10.0f, "%.2fx", "Multiplies amplitude when rear wheels lock harder than front wheels.\nHelps distinguish rear locking (dangerous) from front locking (understeer).");
        }

        ImGui::Separator();
        ImGui::Text("ABS & Hardware");
        ImGui::NextColumn(); ImGui::NextColumn();

        BoolSetting("ABS Pulse", &engine.m_abs_pulse_enabled, "Simulates the pulsing of an ABS system.\nInjects high-frequency pulse when ABS modulates pressure.");
        if (engine.m_abs_pulse_enabled) {
            FloatSetting("  Pulse Gain", &engine.m_abs_gain, 0.0f, 10.0f, "%.2f", "Intensity of the ABS pulse.");
            FloatSetting("  Pulse Frequency", &engine.m_abs_freq_hz, 10.0f, 50.0f, "%.1f Hz", "Rate of the ABS pulse oscillation.");
        }

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::TreeNodeEx("Tactile Textures", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("Texture Load Cap", &engine.m_texture_load_cap, 1.0f, 3.0f, "%.2fx", "Safety Limiter specific to Road and Slide textures.\nPrevents violent shaking when under high downforce or compression.\nONLY affects Road Details and Slide Rumble.");

        BoolSetting("Slide Rumble", &engine.m_slide_texture_enabled, "Vibration proportional to tire sliding/scrubbing velocity.");
        if (engine.m_slide_texture_enabled) {
            FloatSetting("  Slide Gain", &engine.m_slide_texture_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_slide_texture_gain, FFBEngine::BASE_NM_SLIDE_TEXTURE), "Intensity of the scrubbing vibration.");
            FloatSetting("  Slide Pitch", &engine.m_slide_freq_scale, 0.5f, 5.0f, "%.2fx", "Frequency multiplier for the scrubbing sound/feel.\nHigher = Screeching.\nLower = Grinding.");
        }

        BoolSetting("Road Details", &engine.m_road_texture_enabled, "Vibration derived from high-frequency suspension movement.\nFeels road surface, cracks, and bumps.");
        if (engine.m_road_texture_enabled) {
            FloatSetting("  Road Gain", &engine.m_road_texture_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_road_texture_gain, FFBEngine::BASE_NM_ROAD_TEXTURE), "Intensity of road details.");
        }

        BoolSetting("Spin Vibration", &engine.m_spin_enabled, "Vibration when wheels lose traction under acceleration (Wheel Spin).");
        if (engine.m_spin_enabled) {
            FloatSetting("  Spin Strength", &engine.m_spin_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_spin_gain, FFBEngine::BASE_NM_SPIN_VIBRATION), "Intensity of the wheel spin vibration.");
            FloatSetting("  Spin Pitch", &engine.m_spin_freq_scale, 0.5f, 2.0f, "%.2fx", "Scales the frequency of the wheel spin vibration.");
        }

        FloatSetting("Scrub Drag", &engine.m_scrub_drag_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_scrub_drag_gain, FFBEngine::BASE_NM_SCRUB_DRAG), "Constant resistance force when pushing tires laterally (Understeer drag).\nAdds weight to the wheel when scrubbing.");

        const char* bottoming_modes[] = { "Method A: Scraping", "Method B: Susp. Spike" };
        IntSetting("Bottoming Logic", &engine.m_bottoming_method, bottoming_modes, sizeof(bottoming_modes)/sizeof(bottoming_modes[0]), "Algorithm for detecting suspension bottoming.\nScraping = Ride height based.\nSusp Spike = Force rate based.");

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::CollapsingHeader("Advanced Settings")) {
        ImGui::Indent();

        if (ImGui::TreeNode("Stationary Vibration Gate")) {
            float lower_kmh = engine.m_speed_gate_lower * 3.6f;
            if (ImGui::SliderFloat("Mute Below", &lower_kmh, 0.0f, 20.0f, "%.1f km/h")) {
                engine.m_speed_gate_lower = lower_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f)
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("The speed below which all haptic vibrations (Road, Slide, Lockup, Spin) are completely muted to prevent idle shaking.");
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

            float upper_kmh = engine.m_speed_gate_upper * 3.6f;
            if (ImGui::SliderFloat("Full Above", &upper_kmh, 1.0f, 50.0f, "%.1f km/h")) {
                engine.m_speed_gate_upper = upper_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f)
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("The speed above which all haptic vibrations reach their full configured strength.");
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Telemetry Logger")) {
            if (ImGui::Checkbox("Auto-Start on Session", &Config::m_auto_start_logging)) {
                Config::Save(engine);
            }

            char log_path_buf[256];
#ifdef _WIN32
            strncpy_s(log_path_buf, Config::m_log_path.c_str(), 255);
#else
            strncpy(log_path_buf, Config::m_log_path.c_str(), 255);
#endif
            log_path_buf[255] = '\0';
            if (ImGui::InputText("Log Path", log_path_buf, 255)) {
                Config::m_log_path = log_path_buf;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Directory where .csv telemetry logs will be saved.");
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

            if (AsyncLogger::Get().IsLogging()) {
                ImGui::BulletText("Filename: %s", AsyncLogger::Get().GetFilename().c_str());
            }

            ImGui::TreePop();
        }
        ImGui::Unindent();
    }

    ImGui::Columns(1);
    ImGui::End();
}

const float PLOT_HISTORY_SEC = 10.0f;
const int PHYSICS_RATE_HZ = 400;
const int PLOT_BUFFER_SIZE = (int)(PLOT_HISTORY_SEC * PHYSICS_RATE_HZ);

struct RollingBuffer {
    std::vector<float> data;
    int offset = 0;

    RollingBuffer() {
        data.resize(PLOT_BUFFER_SIZE, 0.0f);
    }

    void Add(float val) {
        data[offset] = val;
        offset = (offset + 1) % (int)data.size();
    }

    float GetCurrent() const {
        if (data.empty()) return 0.0f;
        size_t idx = (offset - 1 + (int)data.size()) % (int)data.size();
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

inline void PlotWithStats(const char* label, const RollingBuffer& buffer,
                          float scale_min, float scale_max,
                          const ImVec2& size = ImVec2(0, 40),
                          const char* tooltip = nullptr) {
    ImGui::Text("%s", label);
    char hidden_label[256];
    snprintf(hidden_label, sizeof(hidden_label), "##%s", label);
    ImGui::PlotLines(hidden_label, buffer.data.data(), (int)buffer.data.size(),
                     buffer.offset, NULL, scale_min, scale_max, size);
    if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

    float current = buffer.GetCurrent();
    float min_val = buffer.GetMin();
    float max_val = buffer.GetMax();
    char stats_overlay[128];
    snprintf(stats_overlay, sizeof(stats_overlay), "Cur:%.4f Min:%.3f Max:%.3f", current, min_val, max_val);

    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
    float plot_width = p_max.x - p_min.x;
    p_min.x += 2; p_min.y += 2;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);

    if (text_size.x > plot_width - 4) {
         snprintf(stats_overlay, sizeof(stats_overlay), "%.4f [%.3f, %.3f]", current, min_val, max_val);
         text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         if (text_size.x > plot_width - 4) {
             snprintf(stats_overlay, sizeof(stats_overlay), "Val: %.4f", current);
             text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         }
    }
    draw_list->AddRectFilled(ImVec2(p_min.x - 1, p_min.y), ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y), IM_COL32(0, 0, 0, 90));
    draw_list->AddText(font, font_size, p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
}

// Global Buffers
static RollingBuffer plot_total, plot_base, plot_sop, plot_yaw_kick, plot_rear_torque, plot_gyro_damping, plot_scrub_drag, plot_soft_lock, plot_oversteer, plot_understeer, plot_clipping, plot_road, plot_slide, plot_lockup, plot_spin, plot_bottoming;
static RollingBuffer plot_calc_front_load, plot_calc_rear_load, plot_calc_front_grip, plot_calc_rear_grip, plot_calc_slip_ratio, plot_calc_slip_angle_smoothed, plot_calc_rear_slip_angle_smoothed, plot_slope_current, plot_calc_rear_lat_force;
static RollingBuffer plot_raw_steer, plot_raw_shaft_torque, plot_raw_gen_torque, plot_raw_input_steering, plot_raw_throttle, plot_raw_brake, plot_input_accel, plot_raw_car_speed, plot_raw_load, plot_raw_grip, plot_raw_rear_grip, plot_raw_front_slip_ratio, plot_raw_susp_force, plot_raw_ride_height, plot_raw_front_lat_patch_vel, plot_raw_front_long_patch_vel, plot_raw_rear_lat_patch_vel, plot_raw_rear_long_patch_vel, plot_raw_slip_angle, plot_raw_rear_slip_angle, plot_raw_front_deflection;

static bool g_warn_dt = false;

void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    if (!Config::show_graphs) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + CONFIG_PANEL_WIDTH, viewport->Pos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - CONFIG_PANEL_WIDTH, viewport->Size.y));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("FFB Analysis", nullptr, flags);

    // System Health Diagnostics (Moved from Tuning window - Issue #149)
    if (ImGui::CollapsingHeader("System Health (Hz)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(5, "RateCols", false);
        DisplayRate("FFB Loop", engine.m_ffb_rate, 400.0);
        ImGui::NextColumn();
        DisplayRate("Telemetry", engine.m_telemetry_rate, 400.0);
        ImGui::NextColumn();
        DisplayRate("Hardware", engine.m_hw_rate, 400.0);
        ImGui::NextColumn();
        DisplayRate("S.Torque", engine.m_torque_rate, 400.0);
        ImGui::NextColumn();
        DisplayRate("G.Torque", engine.m_gen_torque_rate, 400.0);
        ImGui::Columns(1);
        if ((engine.m_telemetry_rate < 380.0 || engine.m_torque_rate < 380.0) && engine.m_telemetry_rate > 1.0 && GameConnector::Get().IsConnected()) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Low telemetry/torque rate. Check game FFB settings.");
        }
        ImGui::Separator();
    }

    auto snapshots = engine.GetDebugBatch();
    for (const auto& snap : snapshots) {
        plot_total.Add(snap.total_output);
        plot_base.Add(snap.base_force);
        plot_sop.Add(snap.sop_force);
        plot_yaw_kick.Add(snap.ffb_yaw_kick);
        plot_rear_torque.Add(snap.ffb_rear_torque);
        plot_gyro_damping.Add(snap.ffb_gyro_damping);
        plot_scrub_drag.Add(snap.ffb_scrub_drag);
        plot_soft_lock.Add(snap.ffb_soft_lock);
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
        plot_slope_current.Add(snap.slope_current);
        plot_raw_steer.Add(snap.steer_force);
        plot_raw_shaft_torque.Add(snap.raw_shaft_torque);
        plot_raw_gen_torque.Add(snap.raw_gen_torque);
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
        g_warn_dt = snap.warn_dt;
    }

    if (g_warn_dt) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("TELEMETRY WARNINGS: - Invalid DeltaTime");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("A. FFB Components (Output)", ImGuiTreeNodeFlags_DefaultOpen)) {
        PlotWithStats("Total Output", plot_total, -1.0f, 1.0f, ImVec2(0, 60));
        ImGui::Separator();
        ImGui::Columns(3, "FFBMain", false);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "[Main Forces]");
        PlotWithStats("Base Torque (Nm)", plot_base, -30.0f, 30.0f);
        PlotWithStats("SoP (Chassis G)", plot_sop, -20.0f, 20.0f);
        PlotWithStats("Yaw Kick", plot_yaw_kick, -20.0f, 20.0f);
        PlotWithStats("Rear Align", plot_rear_torque, -20.0f, 20.0f);
        PlotWithStats("Gyro Damping", plot_gyro_damping, -20.0f, 20.0f);
        PlotWithStats("Scrub Drag", plot_scrub_drag, -20.0f, 20.0f);
        PlotWithStats("Soft Lock", plot_soft_lock, -50.0f, 50.0f);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.7f, 1.0f), "[Modifiers]");
        PlotWithStats("Lateral G Boost", plot_oversteer, -20.0f, 20.0f);
        PlotWithStats("Understeer Cut", plot_understeer, -20.0f, 20.0f);
        PlotWithStats("Clipping", plot_clipping, 0.0f, 1.1f);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "[Textures]");
        PlotWithStats("Road Texture", plot_road, -10.0f, 10.0f);
        PlotWithStats("Slide Texture", plot_slide, -10.0f, 10.0f);
        PlotWithStats("Lockup Vib", plot_lockup, -10.0f, 10.0f);
        PlotWithStats("Spin Vib", plot_spin, -10.0f, 10.0f);
        PlotWithStats("Bottoming", plot_bottoming, -10.0f, 10.0f);
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("B. Internal Physics (Brain)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(3, "PhysCols", false);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Loads]");
        ImGui::Text("Front: %.0f N | Rear: %.0f N", plot_calc_front_load.GetCurrent(), plot_calc_rear_load.GetCurrent());
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadF", plot_calc_front_load.data.data(), (int)plot_calc_front_load.data.size(), plot_calc_front_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor();
        ImVec2 pos_load = ImGui::GetItemRectMin();
        ImGui::SetCursorScreenPos(pos_load);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadR", plot_calc_rear_load.data.data(), (int)plot_calc_rear_load.data.size(), plot_calc_rear_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor(2);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Grip/Slip]");
        PlotWithStats("Calc Front Grip", plot_calc_front_grip, 0.0f, 1.2f);
        PlotWithStats("Calc Rear Grip", plot_calc_rear_grip, 0.0f, 1.2f);
        PlotWithStats("Front Slip Ratio", plot_calc_slip_ratio, -1.0f, 1.0f);
        PlotWithStats("Front Slip Angle", plot_calc_slip_angle_smoothed, 0.0f, 1.0f);
        PlotWithStats("Rear Slip Angle", plot_calc_rear_slip_angle_smoothed, 0.0f, 1.0f);
        if (engine.m_slope_detection_enabled) PlotWithStats("Slope", plot_slope_current, -5.0f, 5.0f);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Forces]");
        PlotWithStats("Calc Rear Lat Force", plot_calc_rear_lat_force, -5000.0f, 5000.0f);
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("C. Raw Game Telemetry (Input)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(4, "TelCols", false);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Driver Input]");
        PlotWithStats("Selected Torque", plot_raw_steer, -30.0f, 30.0f, ImVec2(0, 40), "The torque value currently being used as the base for FFB calculations.");
        PlotWithStats("Shaft Torque (100Hz)", plot_raw_shaft_torque, -30.0f, 30.0f, ImVec2(0, 40), "Standard rF2 physics channel (typically 100Hz).");
        PlotWithStats("In-Game FFB (400Hz)", plot_raw_gen_torque, -30.0f, 30.0f, ImVec2(0, 40), "New LMU high-frequency channel (native 400Hz).");
        PlotWithStats("Steering Input", plot_raw_input_steering, -1.0f, 1.0f);
        ImGui::Text("Combined Input");
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PlotLines("##BrkComb", plot_raw_brake.data.data(), (int)plot_raw_brake.data.size(), plot_raw_brake.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(pos);
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PlotLines("##ThrComb", plot_raw_throttle.data.data(), (int)plot_raw_throttle.data.size(), plot_raw_throttle.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::PopStyleColor(2);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Vehicle State]");
        PlotWithStats("Lat Accel", plot_input_accel, -20.0f, 20.0f);
        PlotWithStats("Speed (m/s)", plot_raw_car_speed, 0.0f, 100.0f);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Raw Tire Data]");
        PlotWithStats("Raw Front Load", plot_raw_load, 0.0f, 10000.0f);
        PlotWithStats("Raw Front Grip", plot_raw_grip, 0.0f, 1.2f);
        PlotWithStats("Raw Rear Grip", plot_raw_rear_grip, 0.0f, 1.2f);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Patch Velocities]");
        PlotWithStats("F-Lat PatchVel", plot_raw_front_lat_patch_vel, 0.0f, 20.0f);
        PlotWithStats("R-Lat PatchVel", plot_raw_rear_lat_patch_vel, 0.0f, 20.0f);
        PlotWithStats("F-Long PatchVel", plot_raw_front_long_patch_vel, -20.0f, 20.0f);
        PlotWithStats("R-Long PatchVel", plot_raw_rear_long_patch_vel, -20.0f, 20.0f);
        ImGui::Columns(1);
    }

    ImGui::End();
}
#endif
