#include "GuiLayer.h"
#include "Version.h"
#include "Config.h"
#include "Tooltips.h"
#include "utils/StringUtils.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include "GuiWidgets.h"
#include "AsyncLogger.h"
#include "VehicleUtils.h"
#include "HealthMonitor.h"

#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <chrono>
#include <ctime>
#include <filesystem>

#ifdef ENABLE_IMGUI
#include "imgui.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef LMUFFB_UNIT_TEST
// Linkage for global test access class defined in test_gui_common.h
#include "../../tests/test_gui_common.h"
#endif

using namespace LMUFFB::Logging;
using namespace LMUFFB::Utils;

namespace LMUFFB {
    extern std::atomic<bool> g_running;
    extern std::recursive_mutex g_engine_mutex;
}

namespace LMUFFB {
namespace GUI {

float GuiLayer::m_latest_steering_range = 0.0f;
float GuiLayer::m_latest_steering_angle = 0.0f;
int GuiLayer::m_selected_preset = 0;
bool GuiLayer::m_first_run = true;
std::vector<LMUFFB::FFB::DeviceInfo> GuiLayer::m_devices;
int GuiLayer::m_selected_device_idx = -1;
bool GuiLayer::m_show_save_new_popup = false;
bool GuiLayer::m_show_log_path_popup = false;

#ifdef LMUFFB_UNIT_TEST
std::wstring GuiLayer::m_last_shell_execute_args;
std::string GuiLayer::m_last_system_cmd;
#endif

#ifdef ENABLE_IMGUI
namespace {
    void DisplayRate(const char* label, double rate, double target) {
        ImGui::Text("%s", label);
        static const ImVec4 COLOR_RED(1.0F, 0.4F, 0.4F, 1.0F);
        static const ImVec4 COLOR_GREEN(0.4F, 1.0F, 0.4F, 1.0F);
        static const ImVec4 COLOR_YELLOW(1.0F, 1.0F, 0.4F, 1.0F);
        ImVec4 color = COLOR_RED;
        if (rate >= target * 0.95) color = COLOR_GREEN;
        else if (rate >= target * 0.75) color = COLOR_YELLOW;
        ImGui::TextColored(color, "%.1f Hz", rate);
    }
    constexpr float CONFIG_PANEL_WIDTH = 500.0f;
    constexpr int LATENCY_WARNING_THRESHOLD_MS = 15;
    static constexpr std::chrono::seconds CONNECT_ATTEMPT_INTERVAL(2);

    const float PLOT_HISTORY_SEC = 10.0f;
    const int PHYSICS_RATE_HZ = 400;
    const int PLOT_BUFFER_SIZE = (int)(PLOT_HISTORY_SEC * PHYSICS_RATE_HZ);

    struct RollingBuffer {
        std::vector<float> data;
        int offset = 0;
        RollingBuffer() { data.resize(PLOT_BUFFER_SIZE, 0.0f); }
        void Add(float val) { data[offset] = val; offset = (offset + 1) % (int)data.size(); }
        float GetCurrent() const { if (data.empty()) return 0.0f; return data[(offset - 1 + (int)data.size()) % (int)data.size()]; }
        float GetMin() const { if (data.empty()) return 0.0f; return *std::min_element(data.begin(), data.end()); }
        float GetMax() const { if (data.empty()) return 0.0f; return *std::max_element(data.begin(), data.end()); }
    };

    void PlotWithStats(const char* label, const RollingBuffer& buffer, float scale_min, float scale_max, const ImVec2& size = ImVec2(0, 40), const char* tooltip = nullptr) {
        ImGui::Text("%s", label); char hidden_label[256]; StringUtils::SafeFormat(hidden_label, sizeof(hidden_label), "##%s", label);
        ImGui::PlotLines(hidden_label, buffer.data.data(), (int)buffer.data.size(), buffer.offset, NULL, scale_min, scale_max, size);
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        float current = buffer.GetCurrent(); float min_val = buffer.GetMin(); float max_val = buffer.GetMax();
        char stats_overlay[128]; StringUtils::SafeFormat(stats_overlay, sizeof(stats_overlay), "Cur:%.4f Min:%.3f Max:%.3f", current, min_val, max_val);
        ImVec2 p_min = ImGui::GetItemRectMin(); ImVec2 p_max = ImGui::GetItemRectMax(); float plot_width = p_max.x - p_min.x; p_min.x += 2; p_min.y += 2;
        ImDrawList* draw_list = ImGui::GetWindowDrawList(); ImFont* font = ImGui::GetFont(); float font_size = ImGui::GetFontSize(); ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
        if (text_size.x > plot_width - 4) { StringUtils::SafeFormat(stats_overlay, sizeof(stats_overlay), "%.4f [%.3f, %.3f]", current, min_val, max_val); text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
            if (text_size.x > plot_width - 4) { StringUtils::SafeFormat(stats_overlay, sizeof(stats_overlay), "Val: %.4f", current); text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay); } }
        draw_list->AddRectFilled(ImVec2(p_min.x - 1, p_min.y), ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y), IM_COL32(0, 0, 0, 90));
        draw_list->AddText(font, font_size, p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
    }

    RollingBuffer plot_total, plot_base, plot_sop, plot_yaw_kick, plot_rear_torque, plot_gyro_damping, plot_stationary_damping, plot_scrub_drag, plot_soft_lock, plot_oversteer, plot_understeer, plot_clipping, plot_road, plot_slide, plot_lockup, plot_spin, plot_bottoming;
    RollingBuffer plot_calc_front_load, plot_calc_rear_load, plot_calc_front_grip, plot_calc_rear_grip, plot_calc_slip_ratio, plot_calc_slip_angle_smoothed, plot_calc_rear_slip_angle_smoothed, plot_slope_current, plot_calc_rear_lat_force;
    RollingBuffer plot_raw_steer, plot_raw_shaft_torque, plot_raw_gen_torque, plot_raw_input_steering, plot_raw_throttle, plot_raw_brake, plot_input_accel, plot_raw_car_speed, plot_raw_load, plot_raw_grip, plot_raw_rear_grip, plot_raw_front_slip_ratio, plot_raw_susp_force, plot_raw_ride_height, plot_raw_front_lat_patch_vel, plot_raw_front_long_patch_vel, plot_raw_rear_lat_patch_vel, plot_raw_rear_long_patch_vel, plot_raw_slip_angle, plot_raw_rear_slip_angle, plot_raw_front_deflection;
    bool g_warn_dt = false;
}

// Professional "Flat Dark" Theme
void GuiLayer::SetupGUIStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f; style.ChildRounding = 5.0f; style.PopupRounding = 5.0f; style.FrameRounding = 4.0f; style.GrabRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10); style.FramePadding = ImVec2(8, 4); style.ItemSpacing = ImVec2(8, 6);
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
    colors[ImGuiCol_SliderGrab]     = accent; colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); colors[ImGuiCol_ButtonHovered]  = accent; colors[ImGuiCol_ButtonActive]   = ImVec4(0.00f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_CheckMark]      = accent;
    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_MenuBarBg]      = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
}

void GuiLayer::DrawMenuBar(LMUFFB::FFB::FFBEngine& engine) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("App Settings")) {
            if (ImGui::MenuItem("Always on Top", nullptr, Config::m_always_on_top)) { Config::m_always_on_top = !Config::m_always_on_top; SetWindowAlwaysOnTopPlatform(Config::m_always_on_top); Config::Save(engine); }
            if (ImGui::MenuItem("Enable Graphs", nullptr, Config::show_graphs)) { SaveCurrentWindowGeometryPlatform(Config::show_graphs); Config::show_graphs = !Config::show_graphs; int target_w = Config::show_graphs ? Config::win_w_large : Config::win_w_small; int target_h = Config::show_graphs ? Config::win_h_large : Config::win_h_small; ResizeWindowPlatform(Config::win_pos_x, Config::win_pos_y, target_w, target_h); Config::Save(engine); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Presets")) {
            bool can_save_current = (m_selected_preset >= 0 && m_selected_preset < (int)Config::presets.size() && !Config::presets[m_selected_preset].is_builtin);
            if (ImGui::MenuItem("Save", nullptr, false, can_save_current)) { Config::AddUserPreset(Config::presets[m_selected_preset].name, engine); }
            if (ImGui::MenuItem("Save New...")) { m_show_save_new_popup = true; }
            if (ImGui::MenuItem("Duplicate", nullptr, false, m_selected_preset >= 0)) { Config::DuplicatePreset(m_selected_preset, engine); for (int i = 0; i < (int)Config::presets.size(); i++) { if (Config::presets[i].name == Config::m_last_preset_name) { m_selected_preset = i; break; } } }
            bool can_delete = (m_selected_preset >= 0 && m_selected_preset < (int)Config::presets.size() && !Config::presets[m_selected_preset].is_builtin);
            if (ImGui::MenuItem("Delete", nullptr, false, can_delete)) { Config::DeletePreset(m_selected_preset, engine); m_selected_preset = 0; Config::ApplyPreset(0, engine); }
            ImGui::Separator();
            if (ImGui::MenuItem("Import...")) { std::string path; if (OpenPresetFileDialogPlatform(path)) { if (Config::ImportPreset(path, engine)) m_selected_preset = (int)Config::presets.size() - 1; } }
            if (ImGui::MenuItem("Export...", nullptr, false, m_selected_preset >= 0)) { std::string path; std::string defaultName = Config::presets[m_selected_preset].name + ".ini"; if (SavePresetFileDialogPlatform(path, defaultName)) Config::ExportPreset(m_selected_preset, path); }
            ImGui::Separator();
            auto DrawCatMenu = [&](const char* cat, auto filter) {
                bool has_any = false; for (const auto& pr : Config::presets) { if (filter(pr)) { has_any = true; break; } }
                if (has_any && ImGui::BeginMenu(cat)) {
                    for (int i = 0; i < (int)Config::presets.size(); i++) { const auto& p = Config::presets[i]; if (filter(p)) {
                            std::string n = p.name; if (n.rfind("Guide:", 0) == 0) n = n.substr(n.find(':') + 1); else if (n.rfind("Test:", 0) == 0) n = n.substr(n.find(':') + 1); if (!n.empty() && n[0] == ' ') n.erase(0, 1);
                            ImGui::PushID(i); if (ImGui::MenuItem(n.c_str(), nullptr, m_selected_preset == i)) { m_selected_preset = i; Config::ApplyPreset(i, engine); } ImGui::PopID();
                    } }
                    ImGui::EndMenu();
                }
            };
            DrawCatMenu("User", [](const Preset& p) { return !p.is_builtin; }); DrawCatMenu("Defaults", [](const Preset& p) { return p.is_builtin && p.name.rfind("Guide:", 0) != 0 && p.name.rfind("Test:", 0) != 0; }); DrawCatMenu("Guides", [](const Preset& p) { return p.is_builtin && p.name.rfind("Guide:", 0) == 0; }); DrawCatMenu("Tests", [](const Preset& p) { return p.is_builtin && p.name.rfind("Test:", 0) == 0; });
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Devices")) {
            if (m_devices.empty()) { m_devices = FFB::DirectInputFFB::Get().EnumerateDevices(); if (m_selected_device_idx == -1 && !Config::m_last_device_guid.empty()) { GUID target = FFB::DirectInputFFB::StringToGuid(Config::m_last_device_guid); for (int i = 0; i < (int)m_devices.size(); i++) { if (memcmp(&m_devices[i].guid, &target, sizeof(GUID)) == 0) { m_selected_device_idx = i; FFB::DirectInputFFB::Get().SelectDevice(m_devices[i].guid); break; } } } }
            if (ImGui::MenuItem("Rescan")) { m_devices = FFB::DirectInputFFB::Get().EnumerateDevices(); m_selected_device_idx = -1; }
            if (ImGui::MenuItem("Unbind", nullptr, false, m_selected_device_idx != -1)) { FFB::DirectInputFFB::Get().ReleaseDevice(); m_selected_device_idx = -1; }
            ImGui::Separator();
            for (int i = 0; i < (int)m_devices.size(); i++) { bool is_selected = (m_selected_device_idx == i); if (ImGui::MenuItem(m_devices[i].name.c_str(), nullptr, is_selected)) { m_selected_device_idx = i; FFB::DirectInputFFB::Get().SelectDevice(m_devices[i].guid); Config::m_last_device_guid = FFB::DirectInputFFB::GuidToString(m_devices[i].guid); Config::Save(engine); } }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Logging")) {
            bool is_logging = Config::m_auto_start_logging; if (ImGui::MenuItem(is_logging ? "Stop Auto-Logging" : "Start Auto-Logging", nullptr, is_logging)) { Config::m_auto_start_logging = !is_logging; if (!Config::m_auto_start_logging) AsyncLogger::Get().Stop(); Config::Save(engine); }
            if (ImGui::MenuItem("Set Log Path...")) { m_show_log_path_popup = true; }
            ImGui::Separator();
            if (ImGui::MenuItem("Analyze Last Log")) {
                namespace fs = std::filesystem; fs::path search_path = Config::m_log_path.empty() ? "." : Config::m_log_path; fs::path latest_path; fs::file_time_type latest_time; bool found = false;
                try { if (fs::exists(search_path)) { for (const auto& entry : fs::directory_iterator(search_path)) { if (entry.is_regular_file()) { std::string filename = entry.path().filename().string(); if (filename.find("lmuffb_log_") == 0 && (filename.length() >= 4 && (filename.substr(filename.length()-4) == ".bin" || filename.substr(filename.length()-4) == ".csv"))) { auto ftime = fs::last_write_time(entry); if (!found || ftime > latest_time) { latest_time = ftime; latest_path = entry.path(); found = true; } } } } } } catch (...) {}
                if (found) LaunchLogAnalyzer(latest_path.string());
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void GuiLayer::LaunchLogAnalyzer(const std::string& log_file) {
    namespace fs = std::filesystem; fs::path exe_dir = fs::current_path();
#ifdef _WIN32
    char buffer[MAX_PATH]; if (GetModuleFileNameA(NULL, buffer, MAX_PATH)) exe_dir = fs::path(buffer).parent_path();
#endif
    std::string python_path = (exe_dir / "tools").string(); if (!fs::exists(exe_dir / "tools/lmuffb_log_analyzer")) { if (fs::exists("tools/lmuffb_log_analyzer")) python_path = "tools"; else if (fs::exists("../tools/lmuffb_log_analyzer")) python_path = "../tools"; else if (fs::exists("../../tools/lmuffb_log_analyzer")) python_path = "../../tools"; }
#ifdef _WIN32
    std::wstring wPythonPath = fs::path(python_path).wstring(); SetEnvironmentVariableW(L"PYTHONPATH", wPythonPath.c_str()); std::wstring wLogFile = fs::path(log_file).wstring(); std::wstring wArgs = L"/k python -m lmuffb_log_analyzer.cli analyze-full \"" + wLogFile + L"\"";
#ifdef LMUFFB_UNIT_TEST
    m_last_shell_execute_args = wArgs;
#endif
#ifndef LMUFFB_UNIT_TEST
    ShellExecuteW(NULL, L"open", L"cmd.exe", wArgs.c_str(), NULL, SW_SHOWNORMAL);
#endif
#else
    std::string cmd = "PYTHONPATH=" + python_path + " python3 -m lmuffb_log_analyzer.cli analyze-full \"" + log_file + "\"";
#ifdef LMUFFB_UNIT_TEST
    m_last_system_cmd = cmd;
#endif
#ifndef LMUFFB_UNIT_TEST
    system(cmd.c_str());
#endif
#endif
}

void GuiLayer::DrawTuningWindow(LMUFFB::FFB::FFBEngine& engine) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    if (m_first_run && !Config::presets.empty()) { for (int i = 0; i < (int)Config::presets.size(); i++) { if (Config::presets[i].name == Config::m_last_preset_name) { m_selected_preset = i; break; } } m_first_run = false; }
    ImGuiViewport* viewport = ImGui::GetMainViewport(); float current_width = Config::show_graphs ? CONFIG_PANEL_WIDTH : viewport->WorkSize.x;
    ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(ImVec2(current_width, viewport->WorkSize.y));
    ImGui::Begin("MainUI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    static std::chrono::steady_clock::time_point last_check_time = std::chrono::steady_clock::now();
    if (!LMUFFB::IO::GameConnector::Get().IsConnected()) { ImGui::TextColored(ImVec4(1, 1, 0, 1), "Connecting to LMU..."); if (std::chrono::steady_clock::now() - last_check_time > CONNECT_ATTEMPT_INTERVAL) { last_check_time = std::chrono::steady_clock::now(); LMUFFB::IO::GameConnector::Get().TryConnect(); } } else { ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected to LMU"); }
    if (m_show_save_new_popup) { ImGui::OpenPopup("Save New Preset"); m_show_save_new_popup = false; }
    if (ImGui::BeginPopupModal("Save New Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char buf[64] = ""; if (ImGui::IsWindowAppearing()) { if (m_selected_preset >= 0 && m_selected_preset < (int)Config::presets.size()) StringUtils::SafeCopy(buf, sizeof(buf), Config::presets[m_selected_preset].name.c_str()); else buf[0] = '\0'; }
        ImGui::Text("Enter name for new preset:"); ImGui::InputText("##name", buf, sizeof(buf)); if (ImGui::Button("OK", ImVec2(120, 0))) { if (strlen(buf) > 0) { Config::AddUserPreset(std::string(buf), engine); for (int i = 0; i < (int)Config::presets.size(); i++) { if (Config::presets[i].name == std::string(buf)) { m_selected_preset = i; break; } } ImGui::CloseCurrentPopup(); } }
        ImGui::SameLine(); if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
    }
    if (m_show_log_path_popup) { ImGui::OpenPopup("Set Log Path"); m_show_log_path_popup = false; }
    if (ImGui::BeginPopupModal("Set Log Path", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char buf[256]; if (ImGui::IsWindowAppearing()) StringUtils::SafeCopy(buf, sizeof(buf), Config::m_log_path.c_str());
        ImGui::Text("Enter directory for logs:"); ImGui::InputText("##path", buf, sizeof(buf)); if (ImGui::Button("Save", ImVec2(120, 0))) { Config::m_log_path = buf; Config::Save(engine); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine(); if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
    }
    auto FormatDecoupled = [&](float val, float base_nm) { float estimated_nm = val * base_nm; static char buf[64]; StringUtils::SafeFormat(buf, sizeof(buf), "%.1f%% (~%.1f Nm)", val * 100.0f, estimated_nm); return (const char*)buf; };
    auto FormatPct = [&](float val) { static char buf[32]; StringUtils::SafeFormat(buf, sizeof(buf), "%.1f%%", val * 100.0f); return (const char*)buf; };
    auto FloatSetting = [&](const char* label, float* v, float min, float max, const char* fmt = "%.2f", const char* tooltip = nullptr, std::function<void()> decorator = nullptr) { GuiWidgets::Result res = GuiWidgets::Float(label, v, min, max, fmt, tooltip, decorator); if (res.deactivated) Config::Save(engine); };
    auto BoolSetting = [&](const char* label, bool* v, const char* tooltip = nullptr) { GuiWidgets::Result res = GuiWidgets::Checkbox(label, v, tooltip); if (res.deactivated) Config::Save(engine); };
    if (ImGui::BeginTabBar("TuningTabs")) {
        if (ImGui::BeginTabItem("Old")) { ImGui::Columns(2, "SettingsGridOld", false); if (ImGui::TreeNodeEx("General FFB", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) { ImGui::NextColumn(); ImGui::NextColumn(); ImGui::TextDisabled("Steering: %.1f (%.0f)", m_latest_steering_angle, m_latest_steering_range); ImGui::NextColumn(); ImGui::NextColumn(); BoolSetting("Steerlock from REST API", &engine.m_advanced.rest_api_enabled, Tooltips::REST_API_ENABLE); FloatSetting("Master Gain", &engine.m_general.gain, 0.0f, 2.0f, FormatPct(engine.m_general.gain), Tooltips::MASTER_GAIN); ImGui::TreePop(); } ImGui::NextColumn(); ImGui::NextColumn(); ImGui::Columns(1); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("General")) { ImGui::Indent(); BoolSetting("Steerlock from REST API", &engine.m_advanced.rest_api_enabled, Tooltips::REST_API_ENABLE); ImGui::Separator(); bool use_in_game_ffb = (engine.m_front_axle.torque_source == 1); if (GuiWidgets::Checkbox("Use In-Game FFB (400Hz Native)", &use_in_game_ffb, Tooltips::USE_INGAME_FFB).changed) { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); engine.m_front_axle.torque_source = use_in_game_ffb ? 1 : 0; Config::Save(engine); } BoolSetting("Invert FFB Signal", &engine.m_invert_force, Tooltips::INVERT_FFB); FloatSetting("Master Gain", &engine.m_general.gain, 0.0f, 2.0f, FormatPct(engine.m_general.gain), Tooltips::MASTER_GAIN); FloatSetting("Wheelbase Max Torque", &engine.m_general.wheelbase_max_nm, 1.0f, 50.0f, "%.1f Nm", Tooltips::WHEELBASE_MAX_TORQUE); FloatSetting("Target Rim Torque", &engine.m_general.target_rim_nm, 1.0f, 50.0f, "%.1f Nm", Tooltips::TARGET_RIM_TORQUE); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Front Axle")) { ImGui::Indent(); if (engine.m_front_axle.torque_source == 1) FloatSetting("In-Game FFB Gain", &engine.m_front_axle.ingame_ffb_gain, 0.0f, 2.0f, FormatPct(engine.m_front_axle.ingame_ffb_gain), Tooltips::INGAME_FFB_GAIN); else FloatSetting("Steering Shaft Gain", &engine.m_front_axle.steering_shaft_gain, 0.0f, 2.0f, FormatPct(engine.m_front_axle.steering_shaft_gain), Tooltips::STEERING_SHAFT_GAIN); FloatSetting("Understeer Effect", &engine.m_front_axle.understeer_effect, 0.0f, 2.0f, FormatPct(engine.m_front_axle.understeer_effect), Tooltips::UNDERSTEER_EFFECT); ImGui::Separator(); BoolSetting("Flatspot Suppression", &engine.m_front_axle.flatspot_suppression, Tooltips::FLATSPOT_SUPPRESSION); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Safety")) { ImGui::Indent(); FloatSetting("Safety Duration", &engine.m_safety.m_config.window_duration, 0.0f, 10.0f, "%.1f s", Tooltips::SAFETY_WINDOW_DURATION); FloatSetting("Gain Reduction", &engine.m_safety.m_config.gain_reduction, 0.0f, 1.0f, FormatPct(engine.m_safety.m_config.gain_reduction), Tooltips::SAFETY_GAIN_REDUCTION); BoolSetting("Safety on Stuttering", &engine.m_safety.m_config.stutter_safety_enabled, Tooltips::STUTTER_SAFETY_ENABLE); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Loads")) { ImGui::Indent(); FloatSetting("Lateral Load", &engine.m_load_forces.lat_load_effect, 0.0f, 10.0f, FormatDecoupled(engine.m_load_forces.lat_load_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_LOAD); FloatSetting("Longitudinal G-Force", &engine.m_load_forces.long_load_effect, 0.0f, 10.0f, FormatPct(engine.m_load_forces.long_load_effect), Tooltips::DYNAMIC_WEIGHT); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Rear Axle")) { ImGui::Indent(); FloatSetting("Lateral G Boost (Slide)", &engine.m_rear_axle.oversteer_boost, 0.0f, 4.0f, FormatPct(engine.m_rear_axle.oversteer_boost), Tooltips::OVERSTEER_BOOST); FloatSetting("Lateral G", &engine.m_rear_axle.sop_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_rear_axle.sop_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_G); FloatSetting("SoP Self-Aligning Torque", &engine.m_rear_axle.rear_align_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_rear_axle.rear_align_effect, FFBEngine::BASE_NM_REAR_ALIGN), Tooltips::REAR_ALIGN_TORQUE); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Grip & Slip")) { ImGui::Indent(); BoolSetting("Enable Dynamic Load Sensitivity", &engine.m_grip_estimation.load_sensitivity_enabled, Tooltips::LOAD_SENSITIVITY_ENABLE); FloatSetting("Slip Angle Smoothing", &engine.m_grip_estimation.slip_angle_smoothing, 0.000f, 0.100f, "%.3f s", Tooltips::SLIP_ANGLE_SMOOTHING); BoolSetting("Enable Slope Detection", &engine.m_slope_detection.enabled, Tooltips::SLOPE_DETECTION_ENABLE); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Braking")) { ImGui::Indent(); BoolSetting("Lockup Vibration", &engine.m_braking.lockup_enabled, Tooltips::LOCKUP_VIBRATION); BoolSetting("ABS Pulse", &engine.m_braking.abs_pulse_enabled, Tooltips::ABS_PULSE); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Vibrations")) { ImGui::Indent(); FloatSetting("Vibration Strength", &engine.m_vibration.vibration_gain, 0.0f, 2.0f, FormatPct(engine.m_vibration.vibration_gain), Tooltips::VIBRATION_GAIN); BoolSetting("Slide Rumble", &engine.m_vibration.slide_enabled, Tooltips::SLIDE_RUMBLE); BoolSetting("Road Details", &engine.m_vibration.road_enabled, Tooltips::ROAD_DETAILS); BoolSetting("Spin Vibration", &engine.m_vibration.spin_enabled, Tooltips::SPIN_VIBRATION); ImGui::Unindent(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Advanced")) { ImGui::Indent(); if (ImGui::TreeNode("Stationary Vibration Gate")) { float lower_kmh = engine.m_advanced.speed_gate_lower * 3.6f; if (ImGui::SliderFloat("Mute Below", &lower_kmh, 0.0f, 20.0f, "%.1f km/h")) { engine.m_advanced.speed_gate_lower = lower_kmh / 3.6f; if (engine.m_advanced.speed_gate_upper <= engine.m_advanced.speed_gate_lower + 0.1f) engine.m_advanced.speed_gate_upper = engine.m_advanced.speed_gate_lower + 0.5f; } if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine); ImGui::TreePop(); } ImGui::Unindent(); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void GuiLayer::UpdateTelemetry(LMUFFB::FFB::FFBEngine& engine) {
    auto snapshots = engine.GetDebugBatch();
    for (const auto& snap : snapshots) {
        m_latest_steering_range = snap.steering_range_deg; m_latest_steering_angle = snap.steering_angle_deg;
        plot_total.Add(snap.total_output); plot_base.Add(snap.base_force); plot_sop.Add(snap.sop_force); plot_yaw_kick.Add(snap.ffb_yaw_kick); plot_rear_torque.Add(snap.ffb_rear_torque); plot_gyro_damping.Add(snap.ffb_gyro_damping); plot_stationary_damping.Add(snap.ffb_stationary_damping); plot_scrub_drag.Add(snap.ffb_scrub_drag); plot_soft_lock.Add(snap.ffb_soft_lock); plot_oversteer.Add(snap.oversteer_boost); plot_understeer.Add(snap.understeer_drop); plot_clipping.Add(snap.clipping); plot_road.Add(snap.texture_road); plot_slide.Add(snap.texture_slide); plot_lockup.Add(snap.texture_lockup); plot_spin.Add(snap.texture_spin); plot_bottoming.Add(snap.texture_bottoming);
        plot_calc_front_load.Add(snap.calc_front_load); plot_calc_rear_load.Add(snap.calc_rear_load); plot_calc_front_grip.Add(snap.calc_front_grip); plot_calc_rear_grip.Add(snap.calc_rear_grip); plot_calc_slip_ratio.Add(snap.calc_front_slip_ratio); plot_calc_slip_angle_smoothed.Add(snap.calc_front_slip_angle_smoothed); plot_calc_rear_slip_angle_smoothed.Add(snap.calc_rear_slip_angle_smoothed); plot_calc_rear_lat_force.Add(snap.calc_rear_lat_force); plot_slope_current.Add(snap.slope_current);
        plot_raw_steer.Add(snap.steer_force); plot_raw_shaft_torque.Add(snap.raw_shaft_torque); plot_raw_gen_torque.Add(snap.raw_gen_torque); plot_raw_input_steering.Add(snap.raw_input_steering); plot_raw_throttle.Add(snap.raw_input_throttle); plot_raw_brake.Add(snap.raw_input_brake); plot_input_accel.Add(snap.accel_x); plot_raw_car_speed.Add(snap.raw_car_speed); plot_raw_load.Add(snap.raw_front_tire_load); plot_raw_grip.Add(snap.raw_front_grip_fract); plot_raw_rear_grip.Add(snap.raw_rear_grip); plot_raw_front_slip_ratio.Add(snap.raw_front_slip_ratio); plot_raw_susp_force.Add(snap.raw_front_susp_force); plot_raw_ride_height.Add(snap.raw_front_ride_height); plot_raw_front_lat_patch_vel.Add(snap.raw_front_lat_patch_vel); plot_raw_front_long_patch_vel.Add(snap.raw_front_long_patch_vel); plot_raw_rear_lat_patch_vel.Add(snap.raw_rear_lat_patch_vel); plot_raw_rear_long_patch_vel.Add(snap.raw_rear_long_patch_vel); plot_raw_slip_angle.Add(snap.raw_front_slip_angle); plot_raw_rear_slip_angle.Add(snap.raw_rear_slip_angle); plot_raw_front_deflection.Add(snap.raw_front_deflection); g_warn_dt = snap.warn_dt;
    }
}

void GuiLayer::DrawDebugWindow(LMUFFB::FFB::FFBEngine& engine) {
    if (!Config::show_graphs) return;
    ImGuiViewport* viewport = ImGui::GetMainViewport(); ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + CONFIG_PANEL_WIDTH, viewport->WorkPos.y)); ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x - CONFIG_PANEL_WIDTH, viewport->WorkSize.y));
    ImGui::Begin("FFB Analysis", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    if (ImGui::CollapsingHeader("System Health", ImGuiTreeNodeFlags_DefaultOpen)) {
        HealthStatus hs = HealthMonitor::Check(engine.m_ffb_rate, engine.m_telemetry_rate, engine.m_gen_torque_rate, engine.m_front_axle.torque_source, engine.m_physics_rate, LMUFFB::IO::GameConnector::Get().IsConnected(), LMUFFB::IO::GameConnector::Get().IsSessionActive(), LMUFFB::IO::GameConnector::Get().GetSessionType(), LMUFFB::IO::GameConnector::Get().IsInRealtime(), LMUFFB::IO::GameConnector::Get().GetPlayerControl());
        ImGui::Columns(6, "RateCols", false); DisplayRate("USB Loop", engine.m_ffb_rate, 1000.0); ImGui::NextColumn(); DisplayRate("Physics", engine.m_physics_rate, 400.0); ImGui::NextColumn(); DisplayRate("Telemetry", engine.m_telemetry_rate, 100.0); ImGui::NextColumn(); DisplayRate("Hardware", engine.m_hw_rate, 1000.0); ImGui::NextColumn(); DisplayRate("S.Torque", engine.m_torque_rate, 100.0); ImGui::NextColumn(); DisplayRate("G.Torque", engine.m_gen_torque_rate, 400.0); ImGui::Columns(1);
        ImGui::Separator();
        if (!hs.is_connected) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Sim: Disconnected from LMU");
        else { bool active = hs.session_active; ImGui::TextColored(active ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sim: %s", active ? "Track Loaded" : "Main Menu"); }
        ImGui::Separator();
    }
    if (g_warn_dt) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); ImGui::Text("TELEMETRY WARNINGS: - Invalid DeltaTime"); ImGui::PopStyleColor(); ImGui::Separator(); }
    if (ImGui::CollapsingHeader("FFB Components (Output)", ImGuiTreeNodeFlags_DefaultOpen)) {
        PlotWithStats("Total Output", plot_total, -1.0f, 1.0f, ImVec2(0, 60)); ImGui::Separator();
        ImGui::Columns(3, "FFBMain", false); ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "[Main Forces]");
        PlotWithStats("Base Torque (Nm)", plot_base, -30.0f, 30.0f); PlotWithStats("SoP (Chassis G)", plot_sop, -20.0f, 20.0f); PlotWithStats("Yaw Kick", plot_yaw_kick, -20.0f, 20.0f); PlotWithStats("Rear Align", plot_rear_torque, -20.0f, 20.0f); PlotWithStats("Gyro Damping", plot_gyro_damping, -20.0f, 20.0f); PlotWithStats("Stationary Damping", plot_stationary_damping, -20.0f, 20.0f); PlotWithStats("Scrub Drag", plot_scrub_drag, -20.0f, 20.0f); PlotWithStats("Soft Lock", plot_soft_lock, -50.0f, 50.0f);
        ImGui::NextColumn(); ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.7f, 1.0f), "[Modifiers]"); PlotWithStats("Lateral G Boost", plot_oversteer, -20.0f, 20.0f); PlotWithStats("Understeer Cut", plot_understeer, -20.0f, 20.0f); PlotWithStats("Clipping", plot_clipping, 0.0f, 1.1f);
        ImGui::NextColumn(); ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "[Textures]"); PlotWithStats("Road Texture", plot_road, -10.0f, 10.0f); PlotWithStats("Slide Texture", plot_slide, -10.0f, 10.0f); PlotWithStats("Lockup Vib", plot_lockup, -10.0f, 10.0f); PlotWithStats("Spin Vib", plot_spin, -10.0f, 10.0f); PlotWithStats("Bottoming", plot_bottoming, -10.0f, 10.0f); ImGui::Columns(1);
    }
    ImGui::End();
}
#else
void GuiLayer::DrawMenuBar(LMUFFB::FFB::FFBEngine& engine) {}
void GuiLayer::LaunchLogAnalyzer(const std::string& log_file) {}
void GuiLayer::UpdateTelemetry(LMUFFB::FFB::FFBEngine& engine) {}
void GuiLayer::DrawTuningWindow(LMUFFB::FFB::FFBEngine& engine) {}
void GuiLayer::DrawDebugWindow(LMUFFB::FFB::FFBEngine& engine) {}
void GuiLayer::SetupGUIStyle() {}
#endif

} // namespace GUI
} // namespace LMUFFB

#ifdef LMUFFB_UNIT_TEST
void GuiLayerTestAccess::GetLastLaunchArgs(std::wstring& wArgs, std::string& cmd) {
    wArgs = LMUFFB::GUI::GuiLayer::m_last_shell_execute_args;
    cmd = LMUFFB::GUI::GuiLayer::m_last_system_cmd;
}
#endif
