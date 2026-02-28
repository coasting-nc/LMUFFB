#include "GuiLayer.h"
#include "Version.h"
#include "Config.h"
#include "Tooltips.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include "GuiWidgets.h"
#include "AsyncLogger.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <chrono>
#include <ctime>

#ifdef ENABLE_IMGUI
#include "imgui.h"

static void DisplayRate(const char* label, double rate, double target) {
    ImGui::Text("%s", label);
    
    // Status colors for performance metrics
    static const ImVec4 COLOR_RED(1.0F, 0.4F, 0.4F, 1.0F);
    static const ImVec4 COLOR_GREEN(0.4F, 1.0F, 0.4F, 1.0F);
    static const ImVec4 COLOR_YELLOW(1.0F, 1.0F, 0.4F, 1.0F);

    ImVec4 color = COLOR_RED;
    if (rate >= target * 0.95) {
        color = COLOR_GREEN;
    } else if (rate >= target * 0.75) {
        color = COLOR_YELLOW;
    }
    
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

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.60f, 0.85f, 1.00f));
    ImGui::Text("lmuFFB");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("v%s", LMUFFB_VERSION);
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
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::DEVICE_SELECT);

    ImGui::SameLine();
    if (ImGui::Button("Rescan")) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        selected_device_idx = -1;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::DEVICE_RESCAN);
    ImGui::SameLine();
    if (ImGui::Button("Unbind")) {
        DirectInputFFB::Get().ReleaseDevice();
        selected_device_idx = -1;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::DEVICE_UNBIND);

    if (DirectInputFFB::Get().IsActive()) {
        if (DirectInputFFB::Get().IsExclusive()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Mode: EXCLUSIVE (Game FFB Blocked)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::MODE_EXCLUSIVE);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Mode: SHARED (Potential Conflict)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::MODE_SHARED);
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No device selected.");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::NO_DEVICE);
    }

    if (ImGui::Checkbox("Always on Top", &Config::m_always_on_top)) {
        SetWindowAlwaysOnTopPlatform(Config::m_always_on_top);
        Config::Save(engine);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::ALWAYS_ON_TOP);
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
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::SHOW_GRAPHS);

    ImGui::Separator();
    bool is_logging = AsyncLogger::Get().IsLogging();
    if (is_logging) {
         if (ImGui::Button("STOP LOG", ImVec2(80, 0))) {
             AsyncLogger::Get().Stop();
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::LOG_STOP);
         ImGui::SameLine();
         float time = (float)ImGui::GetTime();
         bool blink = (fmod(time, 1.0f) < 0.5f);
         ImGui::TextColored(blink ? ImVec4(1,0,0,1) : ImVec4(0.6f,0,0,1), "REC");
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::LOG_REC);

         ImGui::SameLine();
         size_t bytes = AsyncLogger::Get().GetFileSizeBytes();
         if (bytes < 1024ULL * 1024ULL)
             ImGui::Text("%zu f (%.0f KB)", AsyncLogger::Get().GetFrameCount(), (float)bytes / 1024.0f);
         else
             ImGui::Text("%zu f (%.1f MB)", AsyncLogger::Get().GetFrameCount(), (float)bytes / (1024.0f * 1024.0f));

         ImGui::SameLine();
         if (ImGui::Button("MARKER")) {
             AsyncLogger::Get().SetMarker();
         }
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::LOG_MARKER);
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
         if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::LOG_START);
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
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_NAME);
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
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_SAVE_NEW);

        if (ImGui::Button("Save Current Config")) {
            if (selected_preset >= 0 && selected_preset < (int)Config::presets.size() && !Config::presets[selected_preset].is_builtin) {
                Config::AddUserPreset(Config::presets[selected_preset].name, engine);
            } else {
                Config::Save(engine);
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_SAVE_CURRENT);
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults")) {
            Config::ApplyPreset(0, engine);
            selected_preset = 0;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_RESET);
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
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_DUPLICATE);
        ImGui::SameLine();
        bool can_delete = (selected_preset >= 0 && selected_preset < (int)Config::presets.size() && !Config::presets[selected_preset].is_builtin);
        if (!can_delete) ImGui::BeginDisabled();
        if (ImGui::Button("Delete")) {
            Config::DeletePreset(selected_preset, engine);
            selected_preset = 0;
            Config::ApplyPreset(0, engine);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_DELETE);
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
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_IMPORT);
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
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::PRESET_EXPORT);

        ImGui::TreePop();
    }

    ImGui::Spacing();

    ImGui::Columns(2, "SettingsGrid", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);

    if (ImGui::TreeNodeEx("General FFB", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Spacing();
        bool use_in_game_ffb = (engine.m_torque_source == 1);
        if (GuiWidgets::Checkbox("Use In-Game FFB (400Hz Native)", &use_in_game_ffb, Tooltips::USE_INGAME_FFB).changed) {
            std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
            engine.m_torque_source = use_in_game_ffb ? 1 : 0;
            Config::Save(engine);
        }

        BoolSetting("Invert FFB Signal", &engine.m_invert_force, Tooltips::INVERT_FFB);
        FloatSetting("Master Gain", &engine.m_gain, 0.0f, 2.0f, FormatPct(engine.m_gain), Tooltips::MASTER_GAIN);
        FloatSetting("Wheelbase Max Torque", &engine.m_wheelbase_max_nm, 1.0f, 50.0f, "%.1f Nm", Tooltips::WHEELBASE_MAX_TORQUE);
        FloatSetting("Target Rim Torque", &engine.m_target_rim_nm, 1.0f, 50.0f, "%.1f Nm", Tooltips::TARGET_RIM_TORQUE);
        FloatSetting("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f", Tooltips::MIN_FORCE);

        if (ImGui::TreeNodeEx("Soft Lock", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::NextColumn(); ImGui::NextColumn();
            BoolSetting("Enable Soft Lock", &engine.m_soft_lock_enabled, Tooltips::SOFT_LOCK_ENABLE);
            if (engine.m_soft_lock_enabled) {
                FloatSetting("  Stiffness", &engine.m_soft_lock_stiffness, 0.0f, 100.0f, "%.1f", Tooltips::SOFT_LOCK_STIFFNESS);
                FloatSetting("  Damping", &engine.m_soft_lock_damping, 0.0f, 5.0f, "%.2f", Tooltips::SOFT_LOCK_DAMPING);
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

        if (engine.m_torque_source == 1) {
            FloatSetting("In-Game FFB Gain", &engine.m_ingame_ffb_gain, 0.0f, 2.0f, FormatPct(engine.m_ingame_ffb_gain), Tooltips::INGAME_FFB_GAIN);
        } else {
            FloatSetting("Steering Shaft Gain", &engine.m_steering_shaft_gain, 0.0f, 2.0f, FormatPct(engine.m_steering_shaft_gain), Tooltips::STEERING_SHAFT_GAIN);
        }

        FloatSetting("Steering Shaft Smoothing", &engine.m_steering_shaft_smoothing, 0.000f, 0.100f, "%.3f s",
            Tooltips::STEERING_SHAFT_SMOOTHING,
            [&]() {
                int ms = (int)std::lround(engine.m_steering_shaft_smoothing * 1000.0f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 2.0f, FormatPct(engine.m_understeer_effect),
            Tooltips::UNDERSTEER_EFFECT);

        FloatSetting("Dynamic Weight", &engine.m_dynamic_weight_gain, 0.0f, 2.0f, FormatPct(engine.m_dynamic_weight_gain),
            Tooltips::DYNAMIC_WEIGHT);

        FloatSetting("  Weight Smoothing", &engine.m_dynamic_weight_smoothing, 0.000f, 0.500f, "%.3f s",
            Tooltips::WEIGHT_SMOOTHING);

        const char* torque_sources[] = { "Shaft Torque (100Hz Legacy)", "In-Game FFB (400Hz LMU 1.2+)" };
        IntSetting("Torque Source", &engine.m_torque_source, torque_sources, sizeof(torque_sources)/sizeof(torque_sources[0]),
            Tooltips::TORQUE_SOURCE);

        BoolSetting("Pure Passthrough", &engine.m_torque_passthrough, Tooltips::PURE_PASSTHROUGH);

        if (ImGui::TreeNodeEx("Signal Filtering", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::NextColumn(); ImGui::NextColumn();

            BoolSetting("  Flatspot Suppression", &engine.m_flatspot_suppression, Tooltips::FLATSPOT_SUPPRESSION);
            if (engine.m_flatspot_suppression) {
                FloatSetting("    Filter Width (Q)", &engine.m_notch_q, 0.5f, 10.0f, "Q: %.2f", Tooltips::NOTCH_Q);
                FloatSetting("    Suppression Strength", &engine.m_flatspot_strength, 0.0f, 1.0f, "%.2f", Tooltips::SUPPRESSION_STRENGTH);
                ImGui::Text("    Est. / Theory Freq");
                ImGui::NextColumn();
                ImGui::TextDisabled("%.1f Hz / %.1f Hz", engine.m_debug_freq, engine.m_theoretical_freq);
                ImGui::NextColumn();
            }

            BoolSetting("  Static Noise Filter", &engine.m_static_notch_enabled, Tooltips::STATIC_NOISE_FILTER);
            if (engine.m_static_notch_enabled) {
                FloatSetting("    Target Frequency", &engine.m_static_notch_freq, 10.0f, 100.0f, "%.1f Hz", Tooltips::STATIC_NOTCH_FREQ);
                FloatSetting("    Filter Width", &engine.m_static_notch_width, 0.1f, 10.0f, "%.1f Hz", Tooltips::STATIC_NOTCH_WIDTH);
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
            Tooltips::OVERSTEER_BOOST);
        FloatSetting("Lateral G", &engine.m_sop_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_sop_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_G);
        FloatSetting("SoP Self-Aligning Torque", &engine.m_rear_align_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_rear_align_effect, FFBEngine::BASE_NM_REAR_ALIGN),
            Tooltips::REAR_ALIGN_TORQUE);
        FloatSetting("Yaw Kick", &engine.m_sop_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_sop_yaw_gain, FFBEngine::BASE_NM_YAW_KICK),
            Tooltips::YAW_KICK);
        FloatSetting("  Activation Threshold", &engine.m_yaw_kick_threshold, 0.0f, 10.0f, "%.2f rad/sÂ²", Tooltips::YAW_KICK_THRESHOLD);

        FloatSetting("  Kick Response", &engine.m_yaw_accel_smoothing, 0.000f, 0.050f, "%.3f s",
            Tooltips::YAW_KICK_RESPONSE,
            [&]() {
                int ms = (int)std::lround(engine.m_yaw_accel_smoothing * 1000.0f);
                ImVec4 color = (ms <= 15) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms", ms);
            });

        FloatSetting("Gyro Damping", &engine.m_gyro_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_gyro_gain, FFBEngine::BASE_NM_GYRO_DAMPING), Tooltips::GYRO_DAMPING);

        FloatSetting("  Gyro Smooth", &engine.m_gyro_smoothing, 0.000f, 0.050f, "%.3f s",
            Tooltips::GYRO_SMOOTH,
            [&]() {
                int ms = (int)std::lround(engine.m_gyro_smoothing * 1000.0f);
                ImVec4 color = (ms <= 20) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms", ms);
            });

        ImGui::TextColored(ImVec4(0.0f, 0.6f, 0.85f, 1.0f), "Advanced SoP");
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f",
            Tooltips::SOP_SMOOTHING,
            [&]() {
                int ms = (int)std::lround((1.0f - engine.m_sop_smoothing_factor) * 100.0f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        FloatSetting("Grip Smoothing", &engine.m_grip_smoothing_steady, 0.000f, 0.100f, "%.3f s",
            Tooltips::GRIP_SMOOTHING);

        FloatSetting("  SoP Scale", &engine.m_sop_scale, 0.0f, 20.0f, "%.2f", Tooltips::SOP_SCALE);

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::TreeNodeEx("Grip & Slip Angle Estimation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("Slip Angle Smoothing", &engine.m_slip_angle_smoothing, 0.000f, 0.100f, "%.3f s",
            Tooltips::SLIP_ANGLE_SMOOTHING,
            [&]() {
                int ms = (int)std::lround(engine.m_slip_angle_smoothing * 1000.0f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        FloatSetting("Chassis Inertia (Load)", &engine.m_chassis_inertia_smoothing, 0.000f, 0.100f, "%.3f s",
            Tooltips::CHASSIS_INERTIA,
            [&]() {
                int ms = (int)std::lround(engine.m_chassis_inertia_smoothing * 1000.0f);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Simulation: %d ms", ms);
            });

        FloatSetting("Optimal Slip Angle", &engine.m_optimal_slip_angle, 0.05f, 0.20f, "%.2f rad",
            Tooltips::OPTIMAL_SLIP_ANGLE);
        FloatSetting("Optimal Slip Ratio", &engine.m_optimal_slip_ratio, 0.05f, 0.20f, "%.2f",
            Tooltips::OPTIMAL_SLIP_RATIO);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Slope Detection (Experimental)");
        ImGui::NextColumn(); ImGui::NextColumn();

        bool prev_slope_enabled = engine.m_slope_detection_enabled;
        GuiWidgets::Result slope_res = GuiWidgets::Checkbox("Enable Slope Detection", &engine.m_slope_detection_enabled,
            Tooltips::SLOPE_DETECTION_ENABLE);

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
                ImGui::SetTooltip("%s", Tooltips::SLOPE_FILTER_WINDOW);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

            ImGui::SameLine();
            float latency_ms = (static_cast<float>(engine.m_slope_sg_window) / 2.0f) * 2.5f;
            ImVec4 color = (latency_ms < 25.0f) ? ImVec4(0,1,0,1) : ImVec4(1,0.5f,0,1);
            ImGui::TextColored(color, "~%.0f ms latency", latency_ms);
            ImGui::NextColumn(); ImGui::NextColumn();

            FloatSetting("  Sensitivity", &engine.m_slope_sensitivity, 0.1f, 5.0f, "%.1fx",
                Tooltips::SLOPE_SENSITIVITY);

            if (ImGui::TreeNode("Advanced Slope Settings")) {
                ImGui::NextColumn(); ImGui::NextColumn();
                FloatSetting("  Slope Threshold", &engine.m_slope_min_threshold, -1.0f, 0.0f, "%.2f", Tooltips::SLOPE_THRESHOLD);
                FloatSetting("  Output Smoothing", &engine.m_slope_smoothing_tau, 0.005f, 0.100f, "%.3f s", Tooltips::SLOPE_OUTPUT_SMOOTHING);

                ImGui::Separator();
                ImGui::Text("Stability Fixes (v0.7.3)");
                ImGui::NextColumn(); ImGui::NextColumn();
                FloatSetting("  Alpha Threshold", &engine.m_slope_alpha_threshold, 0.001f, 0.100f, "%.3f", Tooltips::SLOPE_ALPHA_THRESHOLD);
                FloatSetting("  Decay Rate", &engine.m_slope_decay_rate, 0.5f, 20.0f, "%.1f", Tooltips::SLOPE_DECAY_RATE);
                BoolSetting("  Confidence Gate", &engine.m_slope_confidence_enabled, Tooltips::SLOPE_CONFIDENCE_GATE);

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

        BoolSetting("Lockup Vibration", &engine.m_lockup_enabled, Tooltips::LOCKUP_VIBRATION);
        if (engine.m_lockup_enabled) {
            FloatSetting("  Lockup Strength", &engine.m_lockup_gain, 0.0f, 3.0f, FormatDecoupled(engine.m_lockup_gain, FFBEngine::BASE_NM_LOCKUP_VIBRATION), Tooltips::LOCKUP_STRENGTH);
            FloatSetting("  Brake Load Cap", &engine.m_brake_load_cap, 1.0f, 10.0f, "%.2fx", Tooltips::BRAKE_LOAD_CAP);
            FloatSetting("  Vibration Pitch", &engine.m_lockup_freq_scale, 0.5f, 2.0f, "%.2fx", Tooltips::VIBRATION_PITCH);

            ImGui::Separator();
            ImGui::Text("Response Curve");
            ImGui::NextColumn(); ImGui::NextColumn();

            FloatSetting("  Gamma", &engine.m_lockup_gamma, 0.1f, 3.0f, "%.1f", Tooltips::LOCKUP_GAMMA);
            FloatSetting("  Start Slip %", &engine.m_lockup_start_pct, 1.0f, 10.0f, "%.1f%%", Tooltips::LOCKUP_START_PCT);
            FloatSetting("  Full Slip %", &engine.m_lockup_full_pct, 5.0f, 25.0f, "%.1f%%", Tooltips::LOCKUP_FULL_PCT);

            ImGui::Separator();
            ImGui::Text("Prediction (Advanced)");
            ImGui::NextColumn(); ImGui::NextColumn();

            FloatSetting("  Sensitivity", &engine.m_lockup_prediction_sens, 10.0f, 100.0f, "%.0f", Tooltips::LOCKUP_PREDICTION_SENS);
            FloatSetting("  Bump Rejection", &engine.m_lockup_bump_reject, 0.1f, 5.0f, "%.1f m/s", Tooltips::LOCKUP_BUMP_REJECT);
            FloatSetting("  Rear Boost", &engine.m_lockup_rear_boost, 1.0f, 10.0f, "%.2fx", Tooltips::LOCKUP_REAR_BOOST);
        }

        ImGui::Separator();
        ImGui::Text("ABS & Hardware");
        ImGui::NextColumn(); ImGui::NextColumn();

        BoolSetting("ABS Pulse", &engine.m_abs_pulse_enabled, Tooltips::ABS_PULSE);
        if (engine.m_abs_pulse_enabled) {
            FloatSetting("  Pulse Gain", &engine.m_abs_gain, 0.0f, 10.0f, "%.2f", Tooltips::ABS_PULSE_GAIN);
            FloatSetting("  Pulse Frequency", &engine.m_abs_freq_hz, 10.0f, 50.0f, "%.1f Hz", Tooltips::ABS_PULSE_FREQ);
        }

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    if (ImGui::TreeNodeEx("Tactile Textures", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        FloatSetting("Texture Load Cap", &engine.m_texture_load_cap, 1.0f, 3.0f, "%.2fx", Tooltips::TEXTURE_LOAD_CAP);

        BoolSetting("Slide Rumble", &engine.m_slide_texture_enabled, Tooltips::SLIDE_RUMBLE);
        if (engine.m_slide_texture_enabled) {
            FloatSetting("  Slide Gain", &engine.m_slide_texture_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_slide_texture_gain, FFBEngine::BASE_NM_SLIDE_TEXTURE), Tooltips::SLIDE_GAIN);
            FloatSetting("  Slide Pitch", &engine.m_slide_freq_scale, 0.5f, 5.0f, "%.2fx", Tooltips::SLIDE_PITCH);
        }

        BoolSetting("Road Details", &engine.m_road_texture_enabled, Tooltips::ROAD_DETAILS);
        if (engine.m_road_texture_enabled) {
            FloatSetting("  Road Gain", &engine.m_road_texture_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_road_texture_gain, FFBEngine::BASE_NM_ROAD_TEXTURE), Tooltips::ROAD_GAIN);
        }

        BoolSetting("Spin Vibration", &engine.m_spin_enabled, Tooltips::SPIN_VIBRATION);
        if (engine.m_spin_enabled) {
            FloatSetting("  Spin Strength", &engine.m_spin_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_spin_gain, FFBEngine::BASE_NM_SPIN_VIBRATION), Tooltips::SPIN_STRENGTH);
            FloatSetting("  Spin Pitch", &engine.m_spin_freq_scale, 0.5f, 2.0f, "%.2fx", Tooltips::SPIN_PITCH);
        }

        FloatSetting("Scrub Drag", &engine.m_scrub_drag_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_scrub_drag_gain, FFBEngine::BASE_NM_SCRUB_DRAG), Tooltips::SCRUB_DRAG);

        const char* bottoming_modes[] = { "Method A: Scraping", "Method B: Susp. Spike" };
        IntSetting("Bottoming Logic", &engine.m_bottoming_method, bottoming_modes, sizeof(bottoming_modes)/sizeof(bottoming_modes[0]), Tooltips::BOTTOMING_LOGIC);

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
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::MUTE_BELOW);
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

            float upper_kmh = engine.m_speed_gate_upper * 3.6f;
            if (ImGui::SliderFloat("Full Above", &upper_kmh, 1.0f, 50.0f, "%.1f km/h")) {
                engine.m_speed_gate_upper = upper_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f)
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::FULL_ABOVE);
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Telemetry Logger")) {
            if (ImGui::Checkbox("Auto-Start on Session", &Config::m_auto_start_logging)) {
                Config::Save(engine);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::AUTO_START_LOGGING);

            char log_path_buf[256];
#ifdef _WIN32
            strncpy_s(log_path_buf, sizeof(log_path_buf), Config::m_log_path.c_str(), _TRUNCATE);
#else
            strncpy(log_path_buf, Config::m_log_path.c_str(), 255);
#endif
            log_path_buf[255] = '\0';
            if (ImGui::InputText("Log Path", log_path_buf, 255)) {
                Config::m_log_path = log_path_buf;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::LOG_PATH);
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
        PlotWithStats("Selected Torque", plot_raw_steer, -30.0f, 30.0f, ImVec2(0, 40), Tooltips::PLOT_SELECTED_TORQUE);
        PlotWithStats("Shaft Torque (100Hz)", plot_raw_shaft_torque, -30.0f, 30.0f, ImVec2(0, 40), Tooltips::PLOT_SHAFT_TORQUE);
        PlotWithStats("In-Game FFB (400Hz)", plot_raw_gen_torque, -30.0f, 30.0f, ImVec2(0, 40), Tooltips::PLOT_INGAME_FFB);
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
