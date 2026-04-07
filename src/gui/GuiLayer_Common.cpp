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
#include "implot.h"
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

namespace {

std::string PresetMenuDisplayName(const LMUFFB::Preset& p) {
    std::string n = p.name;
    if (n.rfind("Guide:", 0) == 0) n = n.substr(n.find(':') + 1);
    else if (n.rfind("Test:", 0) == 0) n = n.substr(n.find(':') + 1);
    if (!n.empty() && n[0] == ' ') n.erase(0, 1);
    return n;
}

} // namespace

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

    const int PHYSICS_RATE_HZ = 400;

    struct ScrollingBuffer {
        int MaxSize;
        int Offset;
        std::vector<ImVec2> Data;
        ScrollingBuffer(int max_size = 4000) {
            MaxSize = max_size;
            Offset  = 0;
            Data.reserve(MaxSize);
        }
        void AddPoint(float x, float y) {
            if (Data.size() < (size_t)MaxSize)
                Data.push_back(ImVec2(x,y));
            else {
                Data[Offset] = ImVec2(x,y);
                Offset = (Offset + 1) % MaxSize;
            }
        }
        void Erase() {
            if (Data.size() > 0) {
                Data.shrink_to_fit();
                Data.clear();
            }
            Offset  = 0;
        }
        float GetCurrent() const {
            if (Data.empty()) return 0.0f;
            int last_idx = (Offset - 1 + (int)Data.size()) % (int)Data.size();
            return Data[last_idx].y;
        }
        float GetMin() const {
            if (Data.empty()) return 0.0f;
            float m = Data[0].y;
            for (const auto& p : Data) if (p.y < m) m = p.y;
            return m;
        }
        float GetMax() const {
            if (Data.empty()) return 0.0f;
            float m = Data[0].y;
            for (const auto& p : Data) if (p.y > m) m = p.y;
            return m;
        }
    };

    // Default plot and axis flags  - move into widgets?
    // reference: https://github.com/epezent/implot/blob/master/implot.h#L154
    constexpr ImPlotFlags kDefaultPlotFlags = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect;
    constexpr ImPlotAxisFlags kDefaultXAxisFlags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines;
    constexpr ImPlotAxisFlags kDefaultYAxisFlags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_AutoFit; // | ImPlotAxisFlags_Lock;

    void PlotThis (const char* label, const ScrollingBuffer& buffer,
        float history, float scale_min, float scale_max,
        bool show_stats = false,
        bool show_fill = false,
        const ImVec4* color = nullptr,
        const char* tooltip = nullptr,
        const ImVec2& size = ImVec2(-1, 60),
        const ImPlotFlags plot_flags = kDefaultPlotFlags,
        const ImPlotAxisFlags x_axis_flags = kDefaultXAxisFlags,
        const ImPlotAxisFlags y_axis_flags = kDefaultYAxisFlags) {
        ImGui::Text("%s", label);
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        //ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
        //ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.0f, 0.2f));  // Y only: 0.2 = ImPlot padding factor
        if (ImPlot::BeginPlot(label, size, plot_flags)) {
            ImPlot::SetupAxes(nullptr, nullptr, x_axis_flags, y_axis_flags);
            float current_time = buffer.Data.empty() ? 0.0f : (buffer.Offset == 0 ? buffer.Data.back().x : buffer.Data[buffer.Offset-1].x);
            ImPlot::SetupAxisLimits(ImAxis_X1, current_time - history, current_time, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, (double)scale_min, (double)scale_max, ImGuiCond_None);
            if (color) {
                if (show_fill) {
                    ImPlot::PlotLine("##fill", &buffer.Data[0].x, &buffer.Data[0].y, (int)buffer.Data.size(), ImPlotSpec(ImPlotProp_FillColor, *color, ImPlotProp_FillAlpha, 0.4f, ImPlotProp_Offset, buffer.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float)), ImPlotProp_Flags, ImPlotLineFlags_Shaded));
                }
                ImPlot::PlotLine("##line", &buffer.Data[0].x, &buffer.Data[0].y, (int)buffer.Data.size(), ImPlotSpec(ImPlotProp_LineColor, *color, ImPlotProp_LineWeight, 1.0f, ImPlotProp_Offset, buffer.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float))));
            } else {
                if (show_fill) {
                    ImPlot::PlotLine("##fill", &buffer.Data[0].x, &buffer.Data[0].y, (int)buffer.Data.size(), ImPlotSpec(ImPlotProp_FillColor, ImVec4(1,1,1,1), ImPlotProp_FillAlpha, 0.4f, ImPlotProp_Offset, buffer.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float)), ImPlotProp_Flags, ImPlotLineFlags_Shaded));
                }
                ImPlot::PlotLine("##line", &buffer.Data[0].x, &buffer.Data[0].y, (int)buffer.Data.size(), ImPlotSpec(ImPlotProp_LineColor, ImVec4(1,1,1,1), ImPlotProp_LineWeight, 1.0f, ImPlotProp_Offset, buffer.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float))));
            }
            if (show_stats) {
                float min_val = buffer.GetMin();
                float max_val = buffer.GetMax();
                char stats_overlay[128];
                if (!max_val== 0 && !min_val == 0) {
                    StringUtils::SafeFormat(stats_overlay, sizeof(stats_overlay), "Max: %.2f\nMin:%.2f", max_val, min_val);
                } else { 
                    if (!max_val== 0 && min_val == 0) {StringUtils::SafeFormat(stats_overlay, sizeof(stats_overlay), "Max: %.2f", max_val);
                    } else {StringUtils::SafeFormat(stats_overlay, sizeof(stats_overlay), "Min:%.2f", min_val);}
                }
                ImPlot::Annotation(0, max_val, ImVec4(0,0,0,0.5f), ImVec2(0,0), true, "%s", stats_overlay);
            }
            ImPlot::EndPlot();
        }
        //ImPlot::PopStyleVar(2);
    }

    // Global Buffers
    ScrollingBuffer plot_total, plot_base, plot_sop, plot_yaw_kick, plot_rear_torque, plot_gyro_damping, plot_stationary_damping, plot_scrub_drag, plot_soft_lock, plot_oversteer, plot_understeer, plot_clipping, plot_road, plot_slide, plot_lockup, plot_spin, plot_bottoming;
    ScrollingBuffer plot_calc_front_load, plot_calc_rear_load, plot_calc_front_grip, plot_calc_rear_grip, plot_calc_slip_ratio, plot_calc_slip_angle_smoothed, plot_calc_rear_slip_angle_smoothed, plot_slope_current, plot_calc_rear_lat_force;
    ScrollingBuffer plot_raw_steer, plot_raw_shaft_torque, plot_raw_gen_torque, plot_raw_input_steering, plot_raw_throttle, plot_raw_brake, plot_input_accel, plot_raw_car_speed, plot_raw_load, plot_raw_grip, plot_raw_rear_grip, plot_raw_front_slip_ratio, plot_raw_susp_force, plot_raw_ride_height, plot_raw_front_lat_patch_vel, plot_raw_front_long_patch_vel, plot_raw_rear_lat_patch_vel, plot_raw_rear_long_patch_vel, plot_raw_slip_angle, plot_raw_rear_slip_angle, plot_raw_front_deflection;

    bool g_warn_dt = false;
    float g_plot_time = 0;
    bool g_prev_active = false;
}

// Professional "Flat Dark" Theme
void GuiLayer::SetupGUIStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding            = 5.0f; 
    style.ChildRounding             = 5.0f; 
    style.PopupRounding             = 5.0f; 
    style.FrameRounding             = 4.0f; 
    style.GrabRounding              = 4.0f;
    style.WindowPadding             = ImVec2(10, 10); 
    style.FramePadding              = ImVec2(8, 4); 
    style.ItemSpacing               = ImVec2(8, 6);
    style.TabBarOverlineSize        = 2.0f;
    ImVec4* colors                  = style.Colors;
    ImVec4 accent                   = ImVec4(0.00f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_WindowBg]       = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]        = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg]        = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);
    colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrab]     = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); 
    colors[ImGuiCol_ButtonHovered]  = accent; 
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.00f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_CheckMark]      = accent;
    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); 
    colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_MenuBarBg]      = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.00f, 0.18f, 0.41f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.14f, 0.20f, 0.27f, 1.00f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.00f, 0.18f, 0.41f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
}

void GuiLayer::SetupPlotStyle() {
    ImPlotStyle& style              = ImPlot::GetStyle();
    ImVec4* colors                  = style.Colors;
    /**
    colors[ImPlotCol_LegendText]    = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_TitleText]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_InlayText]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_AxisText]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_Crosshairs]    = ImVec4(0.23f, 0.10f, 0.64f, 0.50f);
    colors[ImPlotCol_PlotBorder]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.MousePosPadding  = ImVec2(5,5);
    **/
    colors[ImPlotCol_PlotBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_Selection] = ImVec4(1.00f, 0.65f, 0.00f, 1.00f);
    style.PlotMinSize      = ImVec2(-1,60);
    style.PlotBorderSize   = 1;
    style.MinorAlpha       = 0.0f;
    style.MajorTickLen     = ImVec2(0,0);
    style.MinorTickLen     = ImVec2(0,0);
    style.MajorTickSize    = ImVec2(0,0);
    style.MinorTickSize    = ImVec2(0,0);
    style.MajorGridSize    = ImVec2(1.2f,1.2f);
    style.MinorGridSize    = ImVec2(1.2f,1.2f);
    style.PlotPadding      = ImVec2(0,0);
    style.LabelPadding     = ImVec2(5,5);
    style.LegendPadding    = ImVec2(5,5);
    style.FitPadding       = ImVec2(0.0f, 0.2f);
    style.DigitalPadding   = 20;
    style.DigitalSpacing   = 4;
    ImPlotSpec spec;
    spec.LineColor         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    spec.LineWeight        = 1.0f;
    spec.FillColor         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    spec.FillAlpha         = 0.5f;
}

void GuiLayer::DrawMenuBar(LMUFFB::FFB::FFBEngine& engine) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    if (m_first_run && !Config::presets.empty()) 
        { for (int i = 0; i < (int)Config::presets.size(); i++) 
            { if (Config::presets[i].name == Config::m_last_preset_name) { m_selected_preset = i; break; } 
        } m_first_run = false; 
    }
    if (m_devices.empty()) {
        m_devices = FFB::DirectInputFFB::Get().EnumerateDevices();
        if (m_selected_device_idx == -1 && !Config::m_last_device_guid.empty()) {
            GUID target = FFB::DirectInputFFB::StringToGuid(Config::m_last_device_guid);
            for (int i = 0; i < (int)m_devices.size(); i++) {
                if (memcmp(&m_devices[i].guid, &target, sizeof(GUID)) == 0) {
                    m_selected_device_idx = i;
                    FFB::DirectInputFFB::Get().SelectDevice(m_devices[i].guid);
                    break;
                }
            }
        }
    }
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Presets")) {
            if (m_selected_preset >= 0 && m_selected_preset < (int)Config::presets.size()) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", PresetMenuDisplayName(Config::presets[m_selected_preset]).c_str());
            }
            bool can_save_current = (m_selected_preset >= 0 && m_selected_preset < (int)Config::presets.size() && !Config::presets[m_selected_preset].is_builtin);
            ImGui::Separator();
            if (ImGui::MenuItem("Save", nullptr, false, can_save_current)) { Config::AddUserPreset(Config::presets[m_selected_preset].name, engine); }
            if (ImGui::MenuItem("Save New...")) { m_show_save_new_popup = true; }
            bool can_delete = (m_selected_preset >= 0 && m_selected_preset < (int)Config::presets.size() && !Config::presets[m_selected_preset].is_builtin);
            if (ImGui::MenuItem("Delete", nullptr, false, can_delete)) { Config::DeletePreset(m_selected_preset, engine); m_selected_preset = 0; Config::ApplyPreset(0, engine); }
            ImGui::Separator();
            if (ImGui::MenuItem("Import...")) { std::string path; if (OpenPresetFileDialogPlatform(path)) { if (Config::ImportPreset(path, engine)) m_selected_preset = (int)Config::presets.size() - 1; Config::ApplyPreset(m_selected_preset, engine);} }
            if (ImGui::MenuItem("Export...", nullptr, false, m_selected_preset >= 0)) { std::string path; std::string defaultName = Config::presets[m_selected_preset].name + ".ini"; if (SavePresetFileDialogPlatform(path, defaultName)) Config::ExportPreset(m_selected_preset, path); }
            ImGui::Separator();
            auto DrawCatMenu = [&](const char* cat, auto filter) {
                bool has_any = false; for (const auto& pr : Config::presets) { if (filter(pr)) { has_any = true; break; } }
                if (has_any && ImGui::BeginMenu(cat)) {
                    for (int i = 0; i < (int)Config::presets.size(); i++) { const auto& p = Config::presets[i]; if (filter(p)) {
                            std::string n = PresetMenuDisplayName(p);
                            ImGui::PushID(i); if (ImGui::MenuItem(n.c_str(), nullptr, m_selected_preset == i)) { m_selected_preset = i; Config::ApplyPreset(i, engine); } ImGui::PopID();
                    } }
                    ImGui::EndMenu();
                }
            };
            DrawCatMenu("User", [](const Preset& p) { return !p.is_builtin; }); DrawCatMenu("Defaults", [](const Preset& p) { return p.is_builtin && p.name.rfind("Guide:", 0) != 0 && p.name.rfind("Test:", 0) != 0; }); DrawCatMenu("Guides", [](const Preset& p) { return p.is_builtin && p.name.rfind("Guide:", 0) == 0; }); DrawCatMenu("Tests", [](const Preset& p) { return p.is_builtin && p.name.rfind("Test:", 0) == 0; });
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Devices")) {
            bool connected = LMUFFB::IO::GameConnector::Get().IsConnected();
            bool exclusive = FFB::DirectInputFFB::Get().IsExclusive();
            ImGui::Text("Game:"); ImGui::SameLine(); ImGui::TextColored(connected ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), connected ? "Connected" : "Disconnected");
            ImGui::Text("FFB:"); ImGui::SameLine(); ImGui::TextColored(exclusive ? ImVec4(0, 1, 0, 1) : ImVec4(1, 1, 0, 1), exclusive ? "Exclusive" : "Shared");
            ImGui::Separator();

            if (ImGui::MenuItem("Rescan")) { m_devices = FFB::DirectInputFFB::Get().EnumerateDevices(); m_selected_device_idx = -1; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::DEVICE_RESCAN);
            if (ImGui::MenuItem("Unbind", nullptr, false, m_selected_device_idx != -1)) { FFB::DirectInputFFB::Get().ReleaseDevice(); m_selected_device_idx = -1; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::DEVICE_UNBIND);
            ImGui::Separator();

            for (int i = 0; i < (int)m_devices.size(); i++) { 
                bool is_selected = (m_selected_device_idx == i); 
                if (ImGui::MenuItem(m_devices[i].name.c_str(), nullptr, is_selected)) 
                    { m_selected_device_idx = i; 
                        FFB::DirectInputFFB::Get().SelectDevice(m_devices[i].guid); 
                        Config::m_last_device_guid = FFB::DirectInputFFB::GuidToString(m_devices[i].guid); 
                        Config::Save(engine); 
                    } 
                };
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::DEVICE_SELECT);
            ImGui::EndMenu();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::DEVICE_SELECT);
        }

        if (ImGui::BeginMenu("Logging")) {
            bool is_logging = Config::m_auto_start_logging; 
            ImGui::Text("Logging:"); ImGui::SameLine(); ImGui::TextColored(is_logging ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), is_logging ? "Auto" : "Disabled");
            if (ImGui::MenuItem(is_logging ? "Stop Auto-Log" : "Start Auto-Log", nullptr, is_logging)) 
                { Config::m_auto_start_logging = !is_logging; if (!Config::m_auto_start_logging) AsyncLogger::Get().Stop(); Config::Save(engine); }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::AUTO_START_LOGGING);
            if (ImGui::MenuItem("Set Log Path...")) { m_show_log_path_popup = true; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::LOG_PATH);
            ImGui::Separator();
            if (ImGui::MenuItem("Analyze Last Log")) {
                namespace fs = std::filesystem; fs::path search_path = Config::m_log_path.empty() ? "." : Config::m_log_path; fs::path latest_path; fs::file_time_type latest_time; bool found = false;
                try { if (fs::exists(search_path)) { 
                    for (const auto& entry : fs::directory_iterator(search_path)) { 
                        if (entry.is_regular_file()) { 
                            std::string filename = entry.path().filename().string(); 
                            if (filename.find("lmuffb_log_") == 0 && (filename.length() >= 4 && (filename.substr(filename.length()-4) == ".bin" || filename.substr(filename.length()-4) == ".csv"))) { 
                                auto ftime = fs::last_write_time(entry); 
                                if (!found || ftime > latest_time) { latest_time = ftime; latest_path = entry.path(); found = true; } 
                            } 
                        } 
                    } 
                } 
            } catch (...) {}
                if (found) LaunchLogAnalyzer(latest_path.string());
            }
            ImGui::EndMenu();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::AUTO_START_LOGGING);
            
        }
        if (ImGui::BeginMenu("UI Settings")) {
            if (ImGui::MenuItem("Always on Top", nullptr, Config::m_always_on_top)) { 
                Config::m_always_on_top = !Config::m_always_on_top; SetWindowAlwaysOnTopPlatform(Config::m_always_on_top); Config::Save(engine); 
            }
            if (ImGui::MenuItem("Enable Graphs", nullptr, Config::show_graphs)) { 
                SaveCurrentWindowGeometryPlatform(Config::show_graphs); 
                Config::show_graphs = !Config::show_graphs; 
                int target_w = Config::show_graphs ? Config::win_w_large : Config::win_w_small; 
                int target_h = Config::show_graphs ? Config::win_h_large : Config::win_h_small; 
                ResizeWindowPlatform(Config::win_pos_x, Config::win_pos_y, target_w, target_h); 
                Config::Save(engine); 
            }
            ImGui::Separator();
            
            int hist = Config::m_graph_history_s;
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::SliderInt("Graph History(s)", &hist, 10, 60)) {
                Config::m_graph_history_s = hist;
                if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set graph history length, range 10-60 seconds");

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void GuiLayer::LaunchLogAnalyzer(const std::string& log_file) {
    namespace fs = std::filesystem;

    // Get executable directory to find tools/ relative to the binary
    fs::path exe_dir = fs::current_path();
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(NULL, buffer, MAX_PATH)) {
        exe_dir = fs::path(buffer).parent_path();
    }
#endif

    // Robust PYTHONPATH lookup
    std::string python_path = (exe_dir / "tools").string();
    if (!fs::exists(exe_dir / "tools/lmuffb_log_analyzer")) {
        // Dev environment fallbacks from CWD
        if (fs::exists("tools/lmuffb_log_analyzer")) python_path = "tools";
        else if (fs::exists("../tools/lmuffb_log_analyzer")) python_path = "../tools";
        else if (fs::exists("../../tools/lmuffb_log_analyzer")) python_path = "../../tools";
    }

    // Windows Defender false positive mitigation :
    // See docs\dev_docs\reports\av_detection_investigation_v0.7.222(pt.2).md
#ifdef _WIN32
    // 1. Set the environment variable natively in the C++ process.
    // cmd.exe and python.exe will automatically inherit this.
    std::wstring wPythonPath = fs::path(python_path).wstring();
    SetEnvironmentVariableW(L"PYTHONPATH", wPythonPath.c_str());

    // 2. Build a clean, unchained command. 
    // /k tells cmd.exe to run the command and KEEP the window open (replacing the need for '& pause')
    std::wstring wLogFile = fs::path(log_file).wstring();
    std::wstring wArgs = L"/k python -m lmuffb_log_analyzer.cli analyze-full \"" + wLogFile + L"\"";

#ifdef LMUFFB_UNIT_TEST
    m_last_shell_execute_args = wArgs;
#endif

#ifndef LMUFFB_UNIT_TEST
    // 3. Execute
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
    ImGuiViewport* viewport = ImGui::GetMainViewport(); float current_width = Config::show_graphs ? CONFIG_PANEL_WIDTH : viewport->WorkSize.x;
    ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(ImVec2(current_width, viewport->WorkSize.y));
    ImGui::Begin("MainUI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    static std::chrono::steady_clock::time_point last_check_time = std::chrono::steady_clock::now();

    if (!LMUFFB::IO::GameConnector::Get().IsConnected()) { ImGui::TextColored(ImVec4(1, 1, 0, 1), "Connecting to LMU..."); 
        if (std::chrono::steady_clock::now() - last_check_time > CONNECT_ATTEMPT_INTERVAL) 
        { last_check_time = std::chrono::steady_clock::now(); LMUFFB::IO::GameConnector::Get().TryConnect(); } 
    } else { ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected to LMU"); }

    if (m_show_save_new_popup) { ImGui::OpenPopup("Save New Preset"); m_show_save_new_popup = false; }
    if (ImGui::BeginPopupModal("Save New Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char buf[64] = ""; 
        if (ImGui::IsWindowAppearing()) 
            { if (m_selected_preset >= 0 && m_selected_preset < (int)Config::presets.size()) StringUtils::SafeCopy(buf, sizeof(buf), Config::presets[m_selected_preset].name.c_str()); else buf[0] = '\0'; }
        ImGui::Text("Enter name for new preset:"); ImGui::InputText("##name", buf, sizeof(buf)); 
        if (ImGui::Button("OK", ImVec2(120, 0))) { if (strlen(buf) > 0) { Config::AddUserPreset(std::string(buf), engine); for (int i = 0; i < (int)Config::presets.size(); i++) 
            { if (Config::presets[i].name == std::string(buf)) { m_selected_preset = i; break; } } ImGui::CloseCurrentPopup(); } }
        ImGui::SameLine(); 
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
    }

    if (m_show_log_path_popup) { ImGui::OpenPopup("Set Log Path"); m_show_log_path_popup = false; }
    if (ImGui::BeginPopupModal("Set Log Path", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char buf[256]; if (ImGui::IsWindowAppearing()) StringUtils::SafeCopy(buf, sizeof(buf), Config::m_log_path.c_str());
        ImGui::Text("Enter directory for logs:"); ImGui::InputText("##path", buf, sizeof(buf)); 
        if (ImGui::Button("Save", ImVec2(120, 0))) { Config::m_log_path = buf; Config::Save(engine); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine(); 
        if (ImGui::Button("Cancel", ImVec2(120, 0))) 
        ImGui::CloseCurrentPopup(); 
        ImGui::EndPopup();
    }

    auto FormatDecoupled = [&](float val, float base_nm) {
        float estimated_nm = val * base_nm;
        static char buf[64];
        StringUtils::SafeFormat(buf, sizeof(buf), "%.1f%%%% (~%.1f Nm)", val * 100.0f, estimated_nm);
        return (const char*)buf;
    };

    auto FormatPct = [&](float val) {
        static char buf[32];
        StringUtils::SafeFormat(buf, sizeof(buf), "%.1f%%%%", val * 100.0f);
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
            Config::Save(engine);
        }
    };
    
    if (ImGui::BeginTabBar("TuningTabs", ImGuiTabBarFlags_DrawSelectedOverline)) {
        if (ImGui::BeginTabItem("General")) { 
            ImGui::Indent();                 
            if (ImGui::BeginTable("GeneralTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                GuiWidgets::ScopedTableLayout table_layout(true);
                ImGui::PushTextWrapPos(0.0f); // Wrap text to the active column/window width.

                GuiWidgets::AdvanceRow();
                BoolSetting("Invert FFB Signal", &engine.m_invert_force, Tooltips::INVERT_FFB);
                bool use_in_game_ffb = (engine.m_front_axle.torque_source == 1);
                if (GuiWidgets::Checkbox("Use In-Game FFB", &use_in_game_ffb, Tooltips::USE_INGAME_FFB).changed) {
                    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); engine.m_front_axle.torque_source = use_in_game_ffb ? 1 : 0;
                    Config::Save(engine);
                }
                if (engine.m_front_axle.torque_source == 1) {
                    ImGui::Indent();
                    FloatSetting("In-Game FFB Gain", &engine.m_front_axle.ingame_ffb_gain, 0.0f, 2.0f, FormatPct(engine.m_front_axle.ingame_ffb_gain), Tooltips::INGAME_FFB_GAIN);
                    ImGui::Unindent();
                }
                if (engine.m_front_axle.torque_source == 0) {
                    ImGui::Indent();
                    const char* recon_modes[] = { "Zero Latency (Extrapolation)", "Smooth (Interpolation)" };
                    IntSetting("Reconstruction", &engine.m_front_axle.steering_100hz_reconstruction, recon_modes, sizeof(recon_modes)/sizeof(recon_modes[0]), Tooltips::STEERING_100HZ_RECONSTRUCTION);
                    ImGui::Unindent();
                }
                BoolSetting("Pure Passthrough", &engine.m_front_axle.torque_passthrough, Tooltips::PURE_PASSTHROUGH);
                
                GuiWidgets::SkipRow(); // Spacer
                FloatSetting("Master Gain", &engine.m_general.gain, 0.0f, 2.0f, FormatPct(engine.m_general.gain), Tooltips::MASTER_GAIN);
                ImGui::Indent();
                FloatSetting("Wheelbase Max Torque", &engine.m_general.wheelbase_max_nm, 1.0f, 50.0f, "%.1f Nm", Tooltips::WHEELBASE_MAX_TORQUE);
                FloatSetting("Target Rim Torque", &engine.m_general.target_rim_nm, 1.0f, 50.0f, "%.1f Nm", Tooltips::TARGET_RIM_TORQUE);
                FloatSetting("Min Force", &engine.m_general.min_force, 0.0f, 0.20f, "%.3f", Tooltips::MIN_FORCE);
                ImGui::Unindent();
                
                GuiWidgets::SkipRow(); // Spacer
                BoolSetting("Steering Soft Lock", &engine.m_advanced.soft_lock_enabled, Tooltips::SOFT_LOCK_ENABLE);
                if (engine.m_advanced.soft_lock_enabled) {
                    GuiWidgets::AdvanceRow();
                    ImGui::Indent();
                    ImGui::TextDisabled("Steering: %.1f° (%.0f)", m_latest_steering_angle, m_latest_steering_range); 
                    ImGui::TableSetColumnIndex(1); ImGui::Dummy(ImVec2(0.0f, 4.0f)); //blank column
                    ImGui::TableNextColumn();
                    BoolSetting("Steerlock from REST API", &engine.m_advanced.rest_api_enabled, Tooltips::REST_API_ENABLE);
                    FloatSetting("Stiffness", &engine.m_advanced.soft_lock_stiffness, 0.0f, 100.0f, "%.1f", Tooltips::SOFT_LOCK_STIFFNESS);
                    FloatSetting("Damping", &engine.m_advanced.soft_lock_damping, 0.0f, 5.0f, "%.2f", Tooltips::SOFT_LOCK_DAMPING);
                    ImGui::Unindent();
                }

                GuiWidgets::SkipRow(); // Spacer
                // Dynamically format the tooltip to show the exact fade-out speed in km/h
                char stat_damp_tooltip[512];
                StringUtils::SafeFormat(stat_damp_tooltip, sizeof(stat_damp_tooltip),
                    "%s\n\nCurrently fades to 0%% at: %.1f km/h (Speed Gate).",
                    Tooltips::STATIONARY_DAMPING, (float)engine.m_advanced.speed_gate_upper * 3.6f);
                FloatSetting("Stationary Damping", &engine.m_advanced.stationary_damping, 0.0f, 0.1f, FormatPct(engine.m_advanced.stationary_damping), stat_damp_tooltip);  //limit max damp to 10% to avoid FFB rattles
                
                GuiWidgets::AdvanceRow(); 
                if (!engine.m_advanced.stationary_damping == 0) {
                    ImGui::Indent();
                    ImGui::TextDisabled("Stationary Enabled:"); ImGui::TableSetColumnIndex(1); float lower_kmh = engine.m_advanced.speed_gate_lower * 3.6f;
                    if (ImGui::SliderFloat("below", &lower_kmh, 0.0f, 20.0f, "%.1f km/h")) { 
                        engine.m_advanced.speed_gate_lower = lower_kmh / 3.6f;
                        if (engine.m_advanced.speed_gate_upper <= engine.m_advanced.speed_gate_lower + 0.1f) engine.m_advanced.speed_gate_upper = engine.m_advanced.speed_gate_lower + 0.5f;
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::MUTE_BELOW);
                    if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);
                    
                    GuiWidgets::AdvanceRow(); ImGui::TextDisabled("Stationary Disabled:"); 
                    ImGui::TableSetColumnIndex(1); 
                    float upper_kmh = engine.m_advanced.speed_gate_upper * 3.6f;
                    if (ImGui::SliderFloat("above", &upper_kmh, 1.0f, 50.0f, "%.1f km/h")) {
                        engine.m_advanced.speed_gate_upper = upper_kmh / 3.6f;
                        if (engine.m_advanced.speed_gate_upper <= engine.m_advanced.speed_gate_lower + 0.1f)
                        engine.m_advanced.speed_gate_upper = engine.m_advanced.speed_gate_lower + 0.5f;
                        }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Tooltips::FULL_ABOVE);
                    if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);
                    ImGui::Unindent();
                }

                GuiWidgets::SkipRow(); // Spacer
                GuiWidgets::SkipRow(); // Spacer
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));   // Text color
                if (ImGui::CollapsingHeader("Advanced settings:", ImGuiTreeNodeFlags_CollapsingHeader)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));   // Text color
                    GuiWidgets::SkipRow(); // Spacer
                    const char* recon_modes[] = { "Zero Latency (Predictive)", "Smooth (Delayed)" };
                    GuiWidgets::Result res = GuiWidgets::Combo("Telemetry Upsampling", &engine.m_advanced.aux_telemetry_reconstruction, recon_modes, 2, Tooltips::AUX_TELEMETRY_RECONSTRUCTION);
                    if (res.changed) {
                        engine.UpdateUpsamplerModes();
                        Config::Save(engine);
                    }

                    GuiWidgets::AdvanceRow();
                    bool prev_structural = engine.m_general.dynamic_normalization_enabled;
                    if (GuiWidgets::Checkbox("FFB Normalization (Session Peak)", &engine.m_general.dynamic_normalization_enabled, Tooltips::DYNAMIC_NORMALIZATION_ENABLE).changed) {
                        if (prev_structural && !engine.m_general.dynamic_normalization_enabled) {
                            engine.ResetNormalization();
                        }
                        Config::Save(engine);
                    }
                    
                    GuiWidgets::AdvanceRow();
                    BoolSetting("Static Noise Filter", &engine.m_front_axle.static_notch_enabled, Tooltips::STATIC_NOISE_FILTER);
                    if (engine.m_front_axle.static_notch_enabled) {
                        ImGui::Indent();
                        FloatSetting("Target Frequency", &engine.m_front_axle.static_notch_freq, 10.0f, 100.0f, "%.1f Hz", Tooltips::STATIC_NOTCH_FREQ);
                        FloatSetting("Filter Width", &engine.m_front_axle.static_notch_width, 0.1f, 10.0f, "%.1f Hz", Tooltips::STATIC_NOTCH_WIDTH);
                        ImGui::Unindent();
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    //ImGui::Text("Spike Protection:");
                    if (ImGui::TreeNode("Spike Protection")) {
                        GuiWidgets::AdvanceRow();
                        BoolSetting("Protect during Stutters", &engine.m_safety.m_config.stutter_safety_enabled, Tooltips::STUTTER_SAFETY_ENABLE);
                        if (engine.m_safety.m_config.stutter_safety_enabled) {
                            FloatSetting("Stutter Threshold", &engine.m_safety.m_config.stutter_threshold, 1.1f, 5.0f, "%.2fx", Tooltips::STUTTER_THRESHOLD, [&]() { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); });
                        }
                        GuiWidgets::AdvanceRow();
                        FloatSetting("Detection Threshold", &engine.m_safety.m_config.spike_detection_threshold, 10.0f, 2000.0f, "%.0f u/s", Tooltips::SPIKE_DETECTION_THRESHOLD, [&]() { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); });
                        FloatSetting("Detection Override", &engine.m_safety.m_config.immediate_spike_threshold, 100.0f, 5000.0f, "%.0f u/s", Tooltips::IMMEDIATE_SPIKE_THRESHOLD, [&]() { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); });
                        FloatSetting("Cut Duration", &engine.m_safety.m_config.window_duration, 0.0f, 10.0f, "%.1f s", Tooltips::SAFETY_WINDOW_DURATION, [&]() { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); });
                        FloatSetting("Cut Strength", &engine.m_safety.m_config.gain_reduction, 0.0f, 1.0f, FormatPct(engine.m_safety.m_config.gain_reduction), Tooltips::SAFETY_GAIN_REDUCTION, [&]() { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); });
                        FloatSetting("Cut Smoothing", &engine.m_safety.m_config.smoothing_tau, 0.001f, 1.0f, "%.3f s", Tooltips::SAFETY_SMOOTHING_TAU, [&]() { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); });
                        FloatSetting("Cut Slew", &engine.m_safety.m_config.slew_full_scale_time_s, 0.1f, 5.0f, "%.2f s", Tooltips::SAFETY_SLEW_FULL_SCALE_TIME_S, [&]() { std::lock_guard<std::recursive_mutex> lock(g_engine_mutex); });
                        ImGui::TreePop();
                    }
                ImGui::PopStyleColor();
                }
            ImGui::PopStyleColor();
            ImGui::PopTextWrapPos();
            ImGui::EndTable();
            }
        ImGui::Unindent();
        ImGui::EndTabItem();
        }; 
        

        if (engine.m_front_axle.torque_passthrough == 0) {
            if (ImGui::BeginTabItem("Steering")) { 
                ImGui::Indent();                 
                if (ImGui::BeginTable("SteeringTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    GuiWidgets::ScopedTableLayout table_layout(true);
                    ImGui::PushTextWrapPos(0.0f); // Wrap text to the active column/window width.
                    GuiWidgets::SkipRow(); // Spacer
                    if (engine.m_front_axle.torque_source == 0) {
                        FloatSetting("Steering Shaft Gain", &engine.m_front_axle.steering_shaft_gain, 0.0f, 2.0f, FormatPct(engine.m_front_axle.steering_shaft_gain), Tooltips::STEERING_SHAFT_GAIN);
                        FloatSetting("Steering Shaft Smoothing", &engine.m_front_axle.steering_shaft_smoothing, 0.000f, 0.100f, "%.3f s", Tooltips::STEERING_SHAFT_SMOOTHING,
                            [&]() {
                                int ms = (int)std::lround(engine.m_front_axle.steering_shaft_smoothing * 1000.0f);
                                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
                            });
                    }
                    FloatSetting("Understeer Effect", &engine.m_front_axle.understeer_effect, 0.0f, 2.0f, FormatPct(engine.m_front_axle.understeer_effect), Tooltips::UNDERSTEER_EFFECT);
                    FloatSetting("Response Curve (Gamma)", &engine.m_front_axle.understeer_gamma, 0.1f, 4.0f, "%.1f", Tooltips::UNDERSTEER_GAMMA);
                    FloatSetting("Lateral G", &engine.m_rear_axle.sop_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_rear_axle.sop_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_G);
                    if (!engine.m_slope_detection.enabled) {
                        FloatSetting("Lateral G Boost (Slide)", &engine.m_rear_axle.oversteer_boost, 0.0f, 4.0f, FormatPct(engine.m_rear_axle.oversteer_boost),Tooltips::OVERSTEER_BOOST);
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    FloatSetting("Self-Align Torque", &engine.m_rear_axle.rear_align_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_rear_axle.rear_align_effect, FFBEngine::BASE_NM_REAR_ALIGN),Tooltips::REAR_ALIGN_TORQUE);
                    
                    if (!engine.m_rear_axle.rear_align_effect == 0) {
                        FloatSetting("Kerb Strike Rejection", &engine.m_rear_axle.kerb_strike_rejection, 0.0f, 1.0f, FormatPct(engine.m_rear_axle.kerb_strike_rejection),Tooltips::KERB_STRIKE_REJECTION);
                        
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    FloatSetting("Yaw Kick", &engine.m_rear_axle.sop_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_rear_axle.sop_yaw_gain, FFBEngine::BASE_NM_YAW_KICK),Tooltips::YAW_KICK);
                   
                    if (!engine.m_rear_axle.sop_yaw_gain == 0) {
                        FloatSetting("   Activation Threshold", &engine.m_rear_axle.yaw_kick_threshold, 0.0f, 10.0f, "%.2f rad/sÂ²", Tooltips::YAW_KICK_THRESHOLD);
                        FloatSetting("   Kick Response", &engine.m_rear_axle.yaw_accel_smoothing, 0.000f, 0.050f, "%.3f s", Tooltips::YAW_KICK_RESPONSE,
                            [&]() {
                                int ms = (int)std::lround(engine.m_rear_axle.yaw_accel_smoothing * 1000.0f);
                                ImVec4 color = (ms <= 15) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                                ImGui::TextColored(color, "Latency: %d ms", ms);
                            });

                        if (ImGui::TreeNodeEx("Unloaded Yaw Kick (Braking)", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::TableNextRow(); ImGui::TableNextColumn();
                            FloatSetting("Gain", &engine.m_rear_axle.unloaded_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_rear_axle.unloaded_yaw_gain, FFBEngine::BASE_NM_YAW_KICK), Tooltips::UNLOADED_YAW_GAIN);
                            if (!engine.m_rear_axle.unloaded_yaw_gain == 0) {
                                FloatSetting("Threshold", &engine.m_rear_axle.unloaded_yaw_threshold, 0.0f, 2.0f, "%.2f rad/sÂ²", Tooltips::UNLOADED_YAW_THRESHOLD);
                                FloatSetting("Unload Sens.", &engine.m_rear_axle.unloaded_yaw_sens, 0.1f, 5.0f, "%.1fx", Tooltips::UNLOADED_YAW_SENS);
                                FloatSetting("Gamma", &engine.m_rear_axle.unloaded_yaw_gamma, 0.1f, 2.0f, "%.1f", Tooltips::UNLOADED_YAW_GAMMA);
                                FloatSetting("Punch (Jerk)", &engine.m_rear_axle.unloaded_yaw_punch, 0.0f, 0.2f, "%.2fx", Tooltips::UNLOADED_YAW_PUNCH);
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNodeEx("Power Yaw Kick (Acceleration)", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::TableNextRow(); ImGui::TableNextColumn();
                            FloatSetting("Gain", &engine.m_rear_axle.power_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_rear_axle.power_yaw_gain, FFBEngine::BASE_NM_YAW_KICK), Tooltips::POWER_YAW_GAIN);
                            if (!engine.m_rear_axle.power_yaw_gain == 0) {
                                FloatSetting("Threshold", &engine.m_rear_axle.power_yaw_threshold, 0.0f, 2.0f, "%.2f rad/sÂ²", Tooltips::POWER_YAW_THRESHOLD);
                                FloatSetting("TC Slip Target", &engine.m_rear_axle.power_slip_threshold, 0.01f, 0.5f, FormatPct(engine.m_rear_axle.power_slip_threshold), Tooltips::POWER_SLIP_THRESHOLD);
                                FloatSetting("Gamma", &engine.m_rear_axle.power_yaw_gamma, 0.1f, 2.0f, "%.1f", Tooltips::POWER_YAW_GAMMA);
                                FloatSetting("Punch (Jerk)", &engine.m_rear_axle.power_yaw_punch, 0.0f, 0.2f, "%.2fx", Tooltips::POWER_YAW_PUNCH);
                            }
                            ImGui::TreePop();
                        }
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    GuiWidgets::SkipRow(); // Spacer
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));   // Text color
                    if (ImGui::CollapsingHeader("Advanced settings:", ImGuiTreeNodeFlags_CollapsingHeader)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));   // Text color

                        //GuiWidgets::SkipRow(); // Spacer
                        //ImGui::TextColored(ImVec4(0.0f, 0.6f, 0.85f, 1.0f), "Advanced SoP");

                        GuiWidgets::SkipRow(); // Spacer
                        FloatSetting("Gyro Damping", &engine.m_advanced.gyro_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_advanced.gyro_gain, FFBEngine::BASE_NM_GYRO_DAMPING), Tooltips::GYRO_DAMPING);
                        FloatSetting("Gyro Smoothing", &engine.m_advanced.gyro_smoothing, 0.000f, 0.050f, "%.3f s", Tooltips::GYRO_SMOOTH,
                            [&]() {
                                int ms = (int)std::lround(engine.m_advanced.gyro_smoothing * 1000.0f);
                                ImVec4 color = (ms <= 20) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                                ImGui::TextColored(color, "Latency: %d ms", ms);
                            });
                        FloatSetting("SoP Gain", &engine.m_rear_axle.sop_scale, 0.0f, 20.0f, "%.2f", Tooltips::SOP_SCALE);
                        FloatSetting("SoP Smoothing", &engine.m_rear_axle.sop_smoothing_factor, 0.0f, 1.0f, "%.2f", Tooltips::SOP_SMOOTHING,
                            [&]() {
                                int ms = (int)std::lround(engine.m_rear_axle.sop_smoothing_factor * 100.0f);
                                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
                            });
                        ImGui::PopStyleColor();
                    }
                ImGui::PopStyleColor();
                ImGui::PopTextWrapPos();
                ImGui::EndTable();
                }   
            ImGui::Unindent(); 
            ImGui::EndTabItem(); 
            }
        
        
            if (ImGui::BeginTabItem("Loads")) { 
                ImGui::Indent();                 
                if (ImGui::BeginTable("LoadSlipTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    GuiWidgets::ScopedTableLayout table_layout(true);
                    ImGui::PushTextWrapPos(0.0f); // Wrap text to the active column/window width.
                    GuiWidgets::SkipRow(); // Spacer

                    FloatSetting("Lateral Load", &engine.m_load_forces.lat_load_effect, 0.0f, 10.0f, FormatDecoupled(engine.m_load_forces.lat_load_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_LOAD);
                    
                    const char* load_transforms[] = { "Linear (Raw)", "Cubic (Smooth)", "Quadratic (Broad)", "Hermite (Locked Center)" };
                    int lat_transform = engine.m_load_forces.lat_load_transform;
                    if (!engine.m_load_forces.lat_load_effect == 0) {
                        ImGui::Indent();
                        if (GuiWidgets::Combo("Lateral Transform", &lat_transform, load_transforms, 4, "Mathematical transformation to soften the lateral load limits and remove 'notchiness'.").changed) {
                            std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                            engine.m_load_forces.lat_load_transform = lat_transform;
                            Config::Save(engine);
                        }
                        ImGui::Unindent();
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    FloatSetting("Longitudinal G-Force", &engine.m_load_forces.long_load_effect, 0.0f, 10.0f, FormatPct(engine.m_load_forces.long_load_effect), Tooltips::DYNAMIC_WEIGHT);
                    if (!engine.m_load_forces.long_load_effect == 0) {
                        ImGui::Indent();
                        FloatSetting("G-Force Smoothing", &engine.m_load_forces.long_load_smoothing, 0.000f, 0.500f, "%.3f s", Tooltips::WEIGHT_SMOOTHING);
                        int long_transform = engine.m_load_forces.long_load_transform;
                        if (GuiWidgets::Combo("G-Force Transform", &long_transform, load_transforms, 4, "Mathematical transformation to soften the longitudinal load limits and remove 'notchiness'.").changed) {
                            std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                            engine.m_load_forces.long_load_transform = long_transform;
                            Config::Save(engine);
                        }
                        ImGui::Unindent();
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    FloatSetting("Chassis Inertia (Load)", &engine.m_grip_estimation.chassis_inertia_smoothing, 0.000f, 0.100f, "%.3f s", Tooltips::CHASSIS_INERTIA,
                        [&]() {
                            int ms = (int)std::lround(engine.m_grip_estimation.chassis_inertia_smoothing * 1000.0f);
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Simulation: %d ms", ms);
                        });
                        
                    GuiWidgets::SkipRow(); // Spacer
                    GuiWidgets::SkipRow(); // Spacer
                    if (GuiWidgets::Checkbox("Dynamic Load Sensitivity", &engine.m_grip_estimation.load_sensitivity_enabled, Tooltips::LOAD_SENSITIVITY_ENABLE).deactivated) {
                        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
                        Config::Save(engine);
                    }

                    bool prev_slope_enabled = engine.m_slope_detection.enabled;
                    GuiWidgets::Result slope_res = GuiWidgets::Checkbox("Dynamic Slope Detection (Experimental)", &engine.m_slope_detection.enabled,Tooltips::SLOPE_DETECTION_ENABLE);
                        if (slope_res.changed) {
                            if (!prev_slope_enabled && engine.m_slope_detection.enabled) {
                                engine.m_slope_buffer_count = 0;
                                engine.m_slope_buffer_index = 0;
                                engine.m_slope_smoothed_output = 1.0;
                            }
                        }
                        if (slope_res.deactivated) {
                            Config::Save(engine);
                        }
                        
                    if (!engine.m_slope_detection.enabled) {
                        GuiWidgets::SkipRow(); // Spacer
                        FloatSetting("Optimal Slip Angle", &engine.m_grip_estimation.optimal_slip_angle, 0.040f, 0.200f, "%.3f rad",Tooltips::OPTIMAL_SLIP_ANGLE);
                        FloatSetting("Optimal Slip Ratio", &engine.m_grip_estimation.optimal_slip_ratio, 0.04f, 0.20f, "%.3f",Tooltips::OPTIMAL_SLIP_RATIO);
                        FloatSetting("Slip Angle Smoothing", &engine.m_grip_estimation.slip_angle_smoothing, 0.000f, 0.100f, "%.3f s",Tooltips::SLIP_ANGLE_SMOOTHING,
                        [&]() {
                            int ms = (int)std::lround(engine.m_grip_estimation.slip_angle_smoothing * 1000.0f);
                            ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                            ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
                        });
                        FloatSetting("Grip Smoothing", &engine.m_grip_estimation.grip_smoothing_steady, 0.000f, 0.100f, "%.3f s", Tooltips::GRIP_SMOOTHING);
                    } else {
                        GuiWidgets::SkipRow(); // Spacer
                        GuiWidgets::SkipRow(); // Spacer
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));   // Text color
                        if (ImGui::CollapsingHeader("Advanced settings:", ImGuiTreeNodeFlags_CollapsingHeader)) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));   // Text color
    
                            GuiWidgets::SkipRow(); // Spacer
                            //ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Slope Detection (Experimental)");
                            //GuiWidgets::SkipRow(); // Spacer
                            BoolSetting("Slope Confidence Gate", &engine.m_slope_detection.confidence_enabled, Tooltips::SLOPE_CONFIDENCE_GATE);
                            FloatSetting("Slope Sensitivity", &engine.m_slope_detection.sensitivity, 0.1f, 5.0f, "%.1fx",Tooltips::SLOPE_SENSITIVITY);
                            FloatSetting("Slope Smoothing", &engine.m_slope_detection.smoothing_tau, 0.005f, 0.100f, "%.3f s", Tooltips::SLOPE_OUTPUT_SMOOTHING);
                            FloatSetting("Slope Decay Rate", &engine.m_slope_detection.decay_rate, 0.5f, 20.0f, "%.1f", Tooltips::SLOPE_DECAY_RATE);
                            FloatSetting("Slope Threshold", &engine.m_slope_detection.min_threshold, -1.0f, 0.0f, "%.2f", Tooltips::SLOPE_THRESHOLD);
                            FloatSetting("Slope Alpha Threshold", &engine.m_slope_detection.alpha_threshold, 0.001f, 0.100f, "%.3f", Tooltips::SLOPE_ALPHA_THRESHOLD);
                            GuiWidgets::AdvanceRow();
                            ImGui::Text("Slope Filter Size");
                            ImGui::TableSetColumnIndex(1);
                            int window = engine.m_slope_detection.sg_window;
                            if (ImGui::SliderInt("w", &window, 5, 41)) {
                                if (window % 2 == 0) window++;
                                engine.m_slope_detection.sg_window = window;
                            }
                            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", Tooltips::SLOPE_FILTER_WINDOW);}
                            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);

                            ImGui::Dummy(ImVec2(0.0f, 4.0f)); ImGui::TableSetColumnIndex(1);
                            float latency_ms = (static_cast<float>(engine.m_slope_detection.sg_window) / 2.0f) * (float)Physics::PHYSICS_CALC_DT * 1000.0f;
                            ImVec4 color = (latency_ms < 25.0f) ? ImVec4(0,1,0,1) : ImVec4(1,0.5f,0,1);
                            ImGui::TextColored(color, "Latency: ~%.0f ms ", latency_ms);
                            GuiWidgets::SkipRow(); // Spacer
                            ImGui::Text("Live Slope: %.3f | Grip: %.0f%%", engine.m_slope_current, engine.m_slope_smoothed_output * 100.0f);
                            ImGui::PopStyleColor();
                        }
                        ImGui::PopStyleColor();
                    }
                ImGui::PopTextWrapPos();
                ImGui::EndTable();
                }
            ImGui::Unindent(); 
            ImGui::EndTabItem(); 
            }
        }
          
        if (ImGui::BeginTabItem("Braking")) { 
                ImGui::Indent();                 
                if (ImGui::BeginTable("BrakingTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    GuiWidgets::ScopedTableLayout table_layout(true);
                    ImGui::PushTextWrapPos(0.0f); // Wrap text to the active column/window width.
                    GuiWidgets::SkipRow(); // Spacer

                    bool prev_vibration_norm = engine.m_general.auto_load_normalization_enabled;  // bug: does not scale vibrations, still need manual vibration gain settings?
                    if (GuiWidgets::Checkbox("Dynamic Load Normalization", &engine.m_general.auto_load_normalization_enabled, Tooltips::DYNAMIC_LOAD_NORMALIZATION_ENABLE).changed) {
                        if (prev_vibration_norm && !engine.m_general.auto_load_normalization_enabled) {
                            engine.ResetNormalization();
                        }
                        Config::Save(engine);
                    }
                    
                    GuiWidgets::SkipRow(); // Spacer
                    BoolSetting("Lockup Vibration", &engine.m_braking.lockup_enabled, Tooltips::LOCKUP_VIBRATION);
                    if (engine.m_braking.lockup_enabled) {
                        ImGui::Indent();
                        if (!engine.m_general.auto_load_normalization_enabled) {
                            FloatSetting("Lockup Gain", &engine.m_braking.lockup_gain, 0.0f, 3.0f, FormatDecoupled(engine.m_braking.lockup_gain, FFBEngine::BASE_NM_LOCKUP_VIBRATION), Tooltips::LOCKUP_STRENGTH);
                        }
                        FloatSetting("Lockup Pitch", &engine.m_braking.lockup_freq_scale, 0.5f, 2.0f, "%.2fx", Tooltips::VIBRATION_PITCH);
                        FloatSetting("Braking Load Gain", &engine.m_braking.brake_load_cap, 1.0f, 10.0f, "%.2fx", Tooltips::BRAKE_LOAD_CAP);
                        FloatSetting("Bump Rejection", &engine.m_braking.lockup_bump_reject, 0.1f, 5.0f, "%.1f m/s", Tooltips::LOCKUP_BUMP_REJECT);
                        ImGui::Unindent(); 
                    }
                    
                    GuiWidgets::SkipRow(); // Spacer
                    BoolSetting("ABS Pulse", &engine.m_braking.abs_pulse_enabled, Tooltips::ABS_PULSE);
                    if (engine.m_braking.abs_pulse_enabled) {
                        ImGui::Indent();
                        FloatSetting("Pulse Gain", &engine.m_braking.abs_gain, 0.0f, 10.0f, "%.2f", Tooltips::ABS_PULSE_GAIN);
                        FloatSetting("Pulse Frequency", &engine.m_braking.abs_freq, 10.0f, 50.0f, "%.1f Hz", Tooltips::ABS_PULSE_FREQ);
                        ImGui::Unindent(); 
                    }
                    
                    GuiWidgets::SkipRow(); // Spacer
                    BoolSetting("Flatspot Suppression", &engine.m_front_axle.flatspot_suppression, Tooltips::FLATSPOT_SUPPRESSION);
                    if (engine.m_front_axle.flatspot_suppression) {
                        ImGui::Indent();
                        FloatSetting("Filter Width (Q)", &engine.m_front_axle.notch_q, 0.5f, 10.0f, "Q: %.2f", Tooltips::NOTCH_Q);
                        FloatSetting("Filter Gain", &engine.m_front_axle.flatspot_strength, 0.0f, 1.0f, "%.2f", Tooltips::SUPPRESSION_STRENGTH);
                        ImGui::Text("Est. / Theory Freq");
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("%.1f Hz / %.1f Hz", engine.m_debug_freq, engine.m_theoretical_freq);
                        ImGui::Unindent();  
                    }
                    
                    if (engine.m_braking.lockup_enabled) {
                        GuiWidgets::SkipRow(); // Spacer
                        GuiWidgets::SkipRow(); // Spacer
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Advanced settings:");
                        ImGui::TableSetColumnIndex(1); ImGui::Dummy(ImVec2(0.0f, 4.0f)); //blank column
                        GuiWidgets::SkipRow(); // Spacer
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Lockup Response/ Prediction:");
                        ImGui::Indent();
                        FloatSetting("Progression Curve", &engine.m_braking.lockup_gamma, 0.1f, 3.0f, "%.1f", Tooltips::LOCKUP_GAMMA);
                        FloatSetting("Minimum Slip %", &engine.m_braking.lockup_start_pct, 1.0f, 10.0f, "%.1f%%", Tooltips::LOCKUP_START_PCT);
                        FloatSetting("Full Slip %", &engine.m_braking.lockup_full_pct, 5.0f, 25.0f, "%.1f%%", Tooltips::LOCKUP_FULL_PCT);
                        FloatSetting("Sensitivity", &engine.m_braking.lockup_prediction_sens, 10.0f, 100.0f, "%.0f", Tooltips::LOCKUP_PREDICTION_SENS);
                        FloatSetting("Rear Lockup Boost", &engine.m_braking.lockup_rear_boost, 1.0f, 10.0f, "%.2fx", Tooltips::LOCKUP_REAR_BOOST);
                        ImGui::Unindent();  
                    }
                    ImGui::PopTextWrapPos();
                    ImGui::EndTable();
                }
                ImGui::Unindent(); 
                ImGui::EndTabItem(); 
        }

        
        if (ImGui::BeginTabItem("Vibrations")) { 
                ImGui::Indent();                 
                if (ImGui::BeginTable("VibrationsTable", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    GuiWidgets::ScopedTableLayout table_layout(true);
                    ImGui::PushTextWrapPos(0.0f); // Wrap text to the active column/window width..
                    GuiWidgets::SkipRow(); // Spacer

                    bool prev_vibration_norm = engine.m_general.auto_load_normalization_enabled; // bug: does not scale vibrations, still need manual vibration gain settings?
                    if (GuiWidgets::Checkbox("Dynamic Load Normalization", &engine.m_general.auto_load_normalization_enabled, Tooltips::DYNAMIC_LOAD_NORMALIZATION_ENABLE).changed) {
                        if (prev_vibration_norm && !engine.m_general.auto_load_normalization_enabled) {
                            engine.ResetNormalization();
                        }
                        Config::Save(engine);
                    }
                    FloatSetting("Vibration Gain", &engine.m_vibration.vibration_gain, 0.0f, 2.0f, FormatPct(engine.m_vibration.vibration_gain), Tooltips::VIBRATION_GAIN);
                    FloatSetting("Vibration Limit", &engine.m_vibration.texture_load_cap, 1.0f, 3.0f, "%.2fx", Tooltips::TEXTURE_LOAD_CAP);

                    GuiWidgets::SkipRow(); // Spacer
                    GuiWidgets::SkipRow(); // Spacer
                    BoolSetting("Slide Rumble", &engine.m_vibration.slide_enabled, Tooltips::SLIDE_RUMBLE);
                    if (engine.m_vibration.slide_enabled) {
                        ImGui::Indent();
                        if (!engine.m_general.auto_load_normalization_enabled) {
                            FloatSetting("Slide Gain", &engine.m_vibration.slide_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_vibration.slide_gain, FFBEngine::BASE_NM_SLIDE_TEXTURE), Tooltips::SLIDE_GAIN);
                        }
                        FloatSetting("Slide Pitch", &engine.m_vibration.slide_freq, 0.5f, 5.0f, "%.2fx", Tooltips::SLIDE_PITCH);
                        ImGui::Unindent();  
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    BoolSetting("Road Details", &engine.m_vibration.road_enabled, Tooltips::ROAD_DETAILS);
                    if (!engine.m_general.auto_load_normalization_enabled) {
                        if (engine.m_vibration.road_enabled) {
                            ImGui::Indent();
                            FloatSetting("Road Gain", &engine.m_vibration.road_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_vibration.road_gain, FFBEngine::BASE_NM_ROAD_TEXTURE), Tooltips::ROAD_GAIN);
                            ImGui::Unindent();  
                        }
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    BoolSetting("Spin Vibration", &engine.m_vibration.spin_enabled, Tooltips::SPIN_VIBRATION);
                    if (engine.m_vibration.spin_enabled) {
                        ImGui::Indent();
                        FloatSetting("Spin Gain", &engine.m_vibration.spin_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_vibration.spin_gain, FFBEngine::BASE_NM_SPIN_VIBRATION), Tooltips::SPIN_STRENGTH);
                        FloatSetting("Spin Pitch", &engine.m_vibration.spin_freq_scale, 0.5f, 2.0f, "%.2fx", Tooltips::SPIN_PITCH);
                        ImGui::Unindent();  
                    }

                    GuiWidgets::SkipRow(); // Spacer
                    FloatSetting("Scrub Drag", &engine.m_vibration.scrub_drag_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_vibration.scrub_drag_gain, FFBEngine::BASE_NM_SCRUB_DRAG), Tooltips::SCRUB_DRAG);
                    
                    GuiWidgets::SkipRow(); // Spacer
                    BoolSetting("Bottoming Effect", &engine.m_vibration.bottoming_enabled, Tooltips::BOTTOMING_EFFECT);
                    if (engine.m_vibration.bottoming_enabled) {
                        FloatSetting("  Bottoming Gain", &engine.m_vibration.bottoming_gain, 0.0f, 2.0f, FormatDecoupled(engine.m_vibration.bottoming_gain, FFBEngine::BASE_NM_BOTTOMING), Tooltips::BOTTOMING_STRENGTH);
                        const char* bottoming_modes[] = { "Method A: Scraping", "Method B: Susp. Spike" };
                        IntSetting("  Bottoming Logic", &engine.m_vibration.bottoming_method, bottoming_modes, sizeof(bottoming_modes)/sizeof(bottoming_modes[0]), Tooltips::BOTTOMING_LOGIC);
                    }
                        
                    ImGui::PopTextWrapPos();
                    ImGui::EndTable();
                }
                ImGui::Unindent(); 
                ImGui::EndTabItem(); 
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}
    
void GuiLayer::UpdateTelemetry(LMUFFB::FFB::FFBEngine& engine) {
    bool is_active = LMUFFB::IO::GameConnector::Get().IsConnected() && LMUFFB::IO::GameConnector::Get().IsInRealtime();

    // Resume detection: Clear state once when game resumes
    if (is_active && !g_prev_active) {
        plot_total.Erase(); plot_base.Erase(); plot_sop.Erase(); plot_yaw_kick.Erase(); plot_rear_torque.Erase();
        plot_gyro_damping.Erase(); plot_stationary_damping.Erase(); plot_scrub_drag.Erase(); plot_soft_lock.Erase();
        plot_oversteer.Erase(); plot_understeer.Erase(); plot_clipping.Erase(); plot_road.Erase(); plot_slide.Erase();
        plot_lockup.Erase(); plot_spin.Erase(); plot_bottoming.Erase();
        plot_calc_front_load.Erase(); plot_calc_rear_load.Erase(); plot_calc_front_grip.Erase(); plot_calc_rear_grip.Erase();
        plot_calc_slip_ratio.Erase(); plot_calc_slip_angle_smoothed.Erase(); plot_calc_rear_slip_angle_smoothed.Erase();
        plot_calc_rear_lat_force.Erase(); plot_slope_current.Erase();
        plot_raw_steer.Erase(); plot_raw_shaft_torque.Erase(); plot_raw_gen_torque.Erase(); plot_raw_input_steering.Erase();
        plot_raw_throttle.Erase(); plot_raw_brake.Erase(); plot_input_accel.Erase(); plot_raw_car_speed.Erase();
        plot_raw_load.Erase(); plot_raw_grip.Erase(); plot_raw_rear_grip.Erase(); plot_raw_front_slip_ratio.Erase();
        plot_raw_susp_force.Erase(); plot_raw_ride_height.Erase(); plot_raw_front_lat_patch_vel.Erase();
        plot_raw_front_long_patch_vel.Erase(); plot_raw_rear_lat_patch_vel.Erase(); plot_raw_rear_long_patch_vel.Erase();
        plot_raw_slip_angle.Erase(); plot_raw_rear_slip_angle.Erase(); plot_raw_front_deflection.Erase();
    }
    g_prev_active = is_active;

    if (!is_active) return; // Freeze at last state

    auto snapshots = engine.GetDebugBatch();
    int buffer_size = Config::m_graph_history_s * PHYSICS_RATE_HZ;

    auto UpdateBuffer = [&](ScrollingBuffer& buf, float val) {
        if (buf.MaxSize != buffer_size) {
            ScrollingBuffer new_buf(buffer_size);
            // Copy existing data if possible, but simplest is just clear/resize for efficiency
            buf = std::move(new_buf);
        }
        buf.AddPoint(g_plot_time, val);
    };

    for (const auto& snap : snapshots) {
        m_latest_steering_range = snap.steering_range_deg;
        m_latest_steering_angle = snap.steering_angle_deg;

        UpdateBuffer(plot_total, snap.total_output);
        UpdateBuffer(plot_base, snap.base_force);
        UpdateBuffer(plot_sop, snap.sop_force);
        UpdateBuffer(plot_yaw_kick, snap.ffb_yaw_kick);
        UpdateBuffer(plot_rear_torque, snap.ffb_rear_torque);
        UpdateBuffer(plot_gyro_damping, snap.ffb_gyro_damping);
        UpdateBuffer(plot_stationary_damping, snap.ffb_stationary_damping);
        UpdateBuffer(plot_scrub_drag, snap.ffb_scrub_drag);
        UpdateBuffer(plot_soft_lock, snap.ffb_soft_lock);
        UpdateBuffer(plot_oversteer, snap.oversteer_boost);
        UpdateBuffer(plot_understeer, snap.understeer_drop);
        UpdateBuffer(plot_clipping, snap.clipping);
        UpdateBuffer(plot_road, snap.texture_road);
        UpdateBuffer(plot_slide, snap.texture_slide);
        UpdateBuffer(plot_lockup, snap.texture_lockup);
        UpdateBuffer(plot_spin, snap.texture_spin);
        UpdateBuffer(plot_bottoming, snap.texture_bottoming);
        UpdateBuffer(plot_calc_front_load, snap.calc_front_load);
        UpdateBuffer(plot_calc_rear_load, snap.calc_rear_load);
        UpdateBuffer(plot_calc_front_grip, snap.calc_front_grip);
        UpdateBuffer(plot_calc_rear_grip, snap.calc_rear_grip);
        UpdateBuffer(plot_calc_slip_ratio, snap.calc_front_slip_ratio);
        UpdateBuffer(plot_calc_slip_angle_smoothed, snap.calc_front_slip_angle_smoothed);
        UpdateBuffer(plot_calc_rear_slip_angle_smoothed, snap.calc_rear_slip_angle_smoothed);
        UpdateBuffer(plot_calc_rear_lat_force, snap.calc_rear_lat_force);
        UpdateBuffer(plot_slope_current, snap.slope_current);
        UpdateBuffer(plot_raw_steer, snap.steer_force);
        UpdateBuffer(plot_raw_shaft_torque, snap.raw_shaft_torque);
        UpdateBuffer(plot_raw_gen_torque, snap.raw_gen_torque);
        UpdateBuffer(plot_raw_input_steering, snap.raw_input_steering);
        UpdateBuffer(plot_raw_throttle, snap.raw_input_throttle);
        UpdateBuffer(plot_raw_brake, snap.raw_input_brake);
        UpdateBuffer(plot_input_accel, snap.accel_x);
        UpdateBuffer(plot_raw_car_speed, snap.raw_car_speed);
        UpdateBuffer(plot_raw_load, snap.raw_front_tire_load);
        UpdateBuffer(plot_raw_grip, snap.raw_front_grip_fract);
        UpdateBuffer(plot_raw_rear_grip, snap.raw_rear_grip);
        UpdateBuffer(plot_raw_front_slip_ratio, snap.raw_front_slip_ratio);
        UpdateBuffer(plot_raw_susp_force, snap.raw_front_susp_force);
        UpdateBuffer(plot_raw_ride_height, snap.raw_front_ride_height);
        UpdateBuffer(plot_raw_front_lat_patch_vel, snap.raw_front_lat_patch_vel);
        UpdateBuffer(plot_raw_front_long_patch_vel, snap.raw_front_long_patch_vel);
        UpdateBuffer(plot_raw_rear_lat_patch_vel, snap.raw_rear_lat_patch_vel);
        UpdateBuffer(plot_raw_rear_long_patch_vel, snap.raw_rear_long_patch_vel);
        UpdateBuffer(plot_raw_slip_angle, snap.raw_front_slip_angle);
        UpdateBuffer(plot_raw_rear_slip_angle, snap.raw_rear_slip_angle);
        UpdateBuffer(plot_raw_front_deflection, snap.raw_front_deflection);
        g_warn_dt = snap.warn_dt;
        g_plot_time += 0.0025f; // 400Hz
    }
}


void GuiLayer::DrawDebugWindow(LMUFFB::FFB::FFBEngine& engine) {
    if (!Config::show_graphs) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + CONFIG_PANEL_WIDTH, viewport->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x - CONFIG_PANEL_WIDTH, viewport->WorkSize.y));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("FFB Analysis", nullptr, flags);

    // System Health Diagnostics (Moved from Tuning window - Issue #149)
    if (ImGui::CollapsingHeader("System Health", ImGuiTreeNodeFlags_DefaultOpen)) {
        HealthStatus hs = HealthMonitor::Check(engine.m_ffb_rate, engine.m_telemetry_rate, engine.m_gen_torque_rate, engine.m_front_axle.torque_source, engine.m_physics_rate,
                                                LMUFFB::IO::GameConnector::Get().IsConnected(), LMUFFB::IO::GameConnector::Get().IsSessionActive(), LMUFFB::IO::GameConnector::Get().GetSessionType(), LMUFFB::IO::GameConnector::Get().IsInRealtime(), LMUFFB::IO::GameConnector::Get().GetPlayerControl());

        ImGui::Columns(6, "RateCols", false);
        DisplayRate("USB Loop", engine.m_ffb_rate, 1000.0);
        ImGui::NextColumn();
        DisplayRate("Physics", engine.m_physics_rate, 400.0);
        ImGui::NextColumn();
        DisplayRate("Telemetry", engine.m_telemetry_rate, 100.0); // Standard LMU is 100Hz
        ImGui::NextColumn();
        DisplayRate("Hardware", engine.m_hw_rate, 1000.0);
        ImGui::NextColumn();
        DisplayRate("S.Torque", engine.m_torque_rate, 100.0);
        ImGui::NextColumn();
        DisplayRate("G.Torque", engine.m_gen_torque_rate, 400.0);
        ImGui::Columns(1);
        
        ImGui::Separator();

        // Robust State Machine (#269, #274)
        if (!hs.is_connected) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Sim: Disconnected from LMU");
        } else {
            bool active = hs.session_active;
            ImGui::TextColored(active ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                "Sim: %s", active ? "Track Loaded" : "Main Menu");
        }

        if (hs.is_connected && hs.session_active) {
            ImGui::SameLine();
            const char* sessionStr = "Unknown";
            long stype = hs.session_type;
            if (stype == 0) sessionStr = "Test Day";
            else if (stype >= 1 && stype <= 4) sessionStr = "Practice";
            else if (stype >= 5 && stype <= 8) sessionStr = "Qualifying";
            else if (stype == 9) sessionStr = "Warmup";
            else if (stype >= 10 && stype <= 13) sessionStr = "Race";
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "| Session: %s", sessionStr);

            bool driving = hs.is_realtime;
            ImGui::TextColored(driving ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 1.0f, 0.4f, 1.0f),
                "State: %s", driving ? "Driving" : "In Menu");

            ImGui::SameLine();
            const char* ctrlStr = "Unknown";
            signed char ctrl = hs.player_control;
            switch (ctrl) {
                case -1: ctrlStr = "Nobody"; break;
                case 0: ctrlStr = "Player"; break;
                case 1: ctrlStr = "AI"; break;
                case 2: ctrlStr = "Remote"; break;
                case 3: ctrlStr = "Replay"; break;
                default: ctrlStr = "Unknown"; break;
            }
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "| Control: %s", ctrlStr);
        }

        //if (!hs.is_healthy && engine.m_telemetry_rate > 1.0 && LMUFFB::IO::GameConnector::Get().IsConnected()) {
        //    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Sub-optimal sample rates detected. Check game settings."); // bug: always on due to mismatch on G.Toque?
        //} 

        ImGui::Separator();
    }


    if (g_warn_dt) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("TELEMETRY WARNINGS: - Invalid DeltaTime");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // Graph history size
    float history = (float)Config::m_graph_history_s;
    // Color presets for Graphs -- todo: move to better location
    static const ImVec4 COLOR_RED(1.0F, 0.4F, 0.4F, 1.0F);
    static const ImVec4 COLOR_GREEN(0.4F, 1.0F, 0.4F, 1.0F);
    static const ImVec4 COLOR_YELLOW(1.0F, 1.0F, 0.4F, 1.0F);
    static const ImVec4 COLOR_CYAN(0.0F, 1.0F, 1.0F, 1.0F);
    static const ImVec4 COLOR_MAGENTA(1.0F, 0.0F, 1.0F, 1.0F);
    static const ImVec4 COLOR_WHITE(1.0F, 1.0F, 1.0F, 1.0F);
    static const ImVec4 COLOR_GREEN2(0.7f, 1.0f, 0.7f, 1.0f);
    static const ImVec4 COLOR_INDIGO(0.7f, 0.7f, 1.0f, 1.0f);

    if (ImGui::CollapsingHeader("A. FFB Components (Output)", ImGuiTreeNodeFlags_DefaultOpen)) {

        PlotThis("Total Output", plot_total, history, -1.0f, 1.0f, true, true, &COLOR_YELLOW, "Total Output", ImVec2(-1, 80));

        ImGui::Separator();
        ImGui::Columns(3, "FFBMain", false);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "[Main Forces]");

        PlotThis("Base Torque (Nm)", plot_base, history, -30.0f, 30.0f, true, true, &COLOR_GREEN);
        PlotThis("SoP (Chassis G)", plot_sop, history, -20.0f, 20.0f, true, true, &COLOR_WHITE);
        if (!engine.m_rear_axle.sop_yaw_gain == 0) {
            PlotThis("Yaw Kick", plot_yaw_kick, history, -20.0f, 20.0f, true, true, &COLOR_WHITE);
        }
        if (!engine.m_rear_axle.rear_align_effect == 0) {
            PlotThis("Rear Align", plot_rear_torque, history, -20.0f, 20.0f, true, true, &COLOR_WHITE);
        }
        if (!engine.m_advanced.gyro_gain == 0) {
            PlotThis("Gyro Damping", plot_gyro_damping, history, -20.0f, 20.0f, true, true, &COLOR_WHITE);
        }
        if (!engine.m_vibration.scrub_drag_gain == 0) {
            PlotThis("Scrub Drag", plot_scrub_drag, history, -20.0f, 20.0f, true, true, &COLOR_INDIGO);
        }

        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.7f, 1.0f), "[Modifiers]");
        PlotThis("Lateral G Boost", plot_oversteer, history, -20.0f, 20.0f, true, true, &COLOR_WHITE);
        PlotThis("Understeer Cut", plot_understeer, history, -20.0f, 20.0f, true, true, &COLOR_WHITE);
        PlotThis("Clipping", plot_clipping, history, 0.0f, 1.1f, false, true, &COLOR_RED);
        if (!engine.m_advanced.stationary_damping == 0) {
            PlotThis("Stationary Damping", plot_stationary_damping, history, -20.0f, 20.0f, false, true, &COLOR_WHITE);
        }
        if (engine.m_advanced.soft_lock_enabled) PlotThis("Soft Lock", plot_soft_lock, history, -50.0f, 50.0f, false, true, &COLOR_INDIGO);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "[Textures]");
        if (engine.m_vibration.road_enabled) PlotThis("Road Texture", plot_road, history, -10.0f, 10.0f, false, false, &COLOR_WHITE);
        if (engine.m_vibration.slide_enabled) PlotThis("Slide Texture", plot_slide, history, -10.0f, 10.0f, false, false, &COLOR_WHITE);
        if (engine.m_vibration.spin_enabled) PlotThis("Spin Vib", plot_spin, history, -10.0f, 10.0f, false, false, &COLOR_WHITE);
        if (engine.m_braking.lockup_enabled) PlotThis("Lockup Vib", plot_lockup, history, -10.0f, 10.0f, false, false, &COLOR_RED);
        if (engine.m_vibration.bottoming_enabled) PlotThis("Bottoming", plot_bottoming, history, -10.0f, 10.0f, true, false, &COLOR_GREEN2);
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("B. Internal Physics (Brain)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(3, "PhysCols", false);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Loads]");
        ImGui::Text("Front: %.0f N | Rear: %.0f N", plot_calc_front_load.GetCurrent(), plot_calc_rear_load.GetCurrent());

        if (ImPlot::BeginPlot("##Loads", ImVec2(-1, 60), ImPlotFlags_NoTitle | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect)) {
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.0f, 0.2f));  // Y only: 0.2 = ImPlot padding factor
            ImPlot::SetupAxes(nullptr, nullptr, kDefaultXAxisFlags, kDefaultYAxisFlags);
            float current_time = plot_calc_front_load.Data.empty() ? 0.0f : (plot_calc_front_load.Offset == 0 ? plot_calc_front_load.Data.back().x : plot_calc_front_load.Data[plot_calc_front_load.Offset-1].x);
            ImPlot::SetupAxisLimits(ImAxis_X1, current_time - history, current_time, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 10000.0f, ImGuiCond_None);
            ImPlot::PlotLine("F", &plot_calc_front_load.Data[0].x, &plot_calc_front_load.Data[0].y, (int)plot_calc_front_load.Data.size(), ImPlotSpec(ImPlotProp_LineColor, COLOR_CYAN, ImPlotProp_Offset, plot_calc_front_load.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float))));
            ImPlot::PlotLine("R", &plot_calc_rear_load.Data[0].x, &plot_calc_rear_load.Data[0].y, (int)plot_calc_rear_load.Data.size(), ImPlotSpec(ImPlotProp_LineColor, COLOR_MAGENTA, ImPlotProp_Offset, plot_calc_rear_load.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float))));
            ImPlot::PopStyleVar(1);
            ImPlot::EndPlot();
        }
        PlotThis("Slope", plot_slope_current, history, -5.0f, 5.0f, true, true, &COLOR_GREEN2);

        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Grip/Slip]");
        PlotThis("Calc Front Grip", plot_calc_front_grip, history, 0.0f, 1.2f, true, true, &COLOR_CYAN);
        PlotThis("Calc Rear Grip", plot_calc_rear_grip, history, 0.0f, 1.2f, true, true, &COLOR_MAGENTA);
        PlotThis("Front Slip Ratio", plot_calc_slip_ratio, history, -1.0f, 1.0f, true, true, &COLOR_WHITE); //bug: no data if raw game data is unavilable
        PlotThis("Front Slip Angle", plot_calc_slip_angle_smoothed, history, 0.0f, 1.0f, true, true, &COLOR_INDIGO);
        PlotThis("Rear Slip Angle", plot_calc_rear_slip_angle_smoothed, history, 0.0f, 1.0f, true, true, &COLOR_INDIGO);

        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Forces]");
        PlotThis("Calc Rear Lat Force", plot_calc_rear_lat_force, history, -5000.0f, 5000.0f, true, true, &COLOR_MAGENTA);
        ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("C. Raw Game Telemetry (Input)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(4, "TelCols", false);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Driver Input]");
        PlotThis("Selected Torque", plot_raw_steer, history, -30.0f, 30.0f, true, true, &COLOR_YELLOW, Tooltips::PLOT_SELECTED_TORQUE);
        if (engine.m_front_axle.torque_source == 0) {
            PlotThis("Shaft Torque (100Hz)", plot_raw_shaft_torque, history, -30.0f, 30.0f, false, false, &COLOR_YELLOW, Tooltips::PLOT_SHAFT_TORQUE);
        } else {
            PlotThis("In-Game FFB (400Hz)", plot_raw_gen_torque, history, -30.0f, 30.0f, false, false, &COLOR_YELLOW, Tooltips::PLOT_INGAME_FFB);
        }
        PlotThis("Steering Input", plot_raw_input_steering, history, -1.0f, 1.0f, false, true, &COLOR_WHITE); 

        ImGui::Text("Combined Input");
        if (ImPlot::BeginPlot("##InputComb", ImVec2(-1, 90), kDefaultPlotFlags)) {
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.0f, 1.0f));  // Y only: 0.2 = ImPlot padding factor
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoSideSwitch | ImPlotAxisFlags_Foreground);
            float current_time = plot_raw_throttle.Data.empty() ? 0.0f : (plot_raw_throttle.Offset == 0 ? plot_raw_throttle.Data.back().x : plot_raw_throttle.Data[plot_raw_throttle.Offset-1].x);
            ImPlot::SetupAxisLimits(ImAxis_X1, current_time - 20, current_time, ImGuiCond_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -0.06f, 1.06f, ImGuiCond_Always);
            ImPlot::PlotLine("##BrakeFill", &plot_raw_brake.Data[0].x, &plot_raw_brake.Data[0].y, (int)plot_raw_brake.Data.size(), ImPlotSpec(ImPlotProp_FillColor, COLOR_RED, ImPlotProp_FillAlpha, 1.0f, ImPlotProp_Offset, plot_raw_brake.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float)), ImPlotProp_Flags, ImPlotLineFlags_Shaded));
            ImPlot::PlotLine("##ThrottleFill", &plot_raw_throttle.Data[0].x, &plot_raw_throttle.Data[0].y, (int)plot_raw_throttle.Data.size(), ImPlotSpec(ImPlotProp_FillColor, COLOR_GREEN, ImPlotProp_FillAlpha, 1.0f, ImPlotProp_Offset, plot_raw_throttle.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float)), ImPlotProp_Flags, ImPlotLineFlags_Shaded));
            ImPlot::PlotLine("##Throttle", &plot_raw_throttle.Data[0].x, &plot_raw_throttle.Data[0].y, (int)plot_raw_throttle.Data.size(), ImPlotSpec(ImPlotProp_LineColor, COLOR_GREEN, ImPlotProp_LineWeight, 2.0f, ImPlotProp_Offset, plot_raw_throttle.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float))));
            ImPlot::PlotLine("##Brake", &plot_raw_brake.Data[0].x, &plot_raw_brake.Data[0].y, (int)plot_raw_brake.Data.size(), ImPlotSpec(ImPlotProp_LineColor, COLOR_RED, ImPlotProp_LineWeight, 2.0f, ImPlotProp_Offset, plot_raw_brake.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float))));
            ImPlot::PopStyleVar(1);
            ImPlot::EndPlot();
        }

        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Vehicle State]");
        PlotThis("Lat Accel", plot_input_accel, history, -20.0f, 20.0f, true, true, &COLOR_GREEN2);
        //PlotThis("Speed (m/s)", plot_raw_car_speed, history, 0.0f, 100.0f, true, true, &COLOR_GREEN2);  //bug?: data stream inverted?  
        ImGui::Text("Speed (m/s)");
        if (ImPlot::BeginPlot("Speed (m/s)", ImVec2(-1, 60), kDefaultPlotFlags)) {
            ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
            ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.0f, 0.2f));  // Y only: 0.2 = ImPlot padding factor
            ImPlot::SetupAxes(nullptr, nullptr, kDefaultXAxisFlags, kDefaultYAxisFlags | ImPlotAxisFlags_Invert);
            float current_time = plot_raw_car_speed.Data.empty() ? 0.0f : (plot_raw_car_speed.Offset == 0 ? plot_raw_car_speed.Data.back().x : plot_raw_car_speed.Data[plot_raw_car_speed.Offset-1].x);
            ImPlot::SetupAxisLimits(ImAxis_X1, current_time - history, current_time, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100.0f, ImGuiCond_None);
            ImPlot::PlotLine("##Speedms", &plot_raw_car_speed.Data[0].x, &plot_raw_car_speed.Data[0].y, (int)plot_raw_car_speed.Data.size(), ImPlotSpec(ImPlotProp_LineColor, COLOR_GREEN2, ImPlotProp_Offset, plot_raw_car_speed.Offset, ImPlotProp_Stride, (int)(2 * sizeof(float))));
            float max_val = plot_raw_car_speed.GetMax();
            char stats_overlay[128];
            StringUtils::SafeFormat(stats_overlay, sizeof(stats_overlay), "Max: %.2f", max_val);
            ImPlot::Annotation(0, 100, ImVec4(0,0,0,0.5f), ImVec2(0,0), true, "%s", stats_overlay);
            ImPlot::PopStyleVar(1);
            ImPlot::EndPlot();
        }
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Raw Tire Data]");
        PlotThis("Raw Front Load", plot_raw_load, history, 0.0f, 10000.0f, true, true, &COLOR_WHITE);
        PlotThis("Raw Front Grip", plot_raw_grip, history, 0.0f, 1.2f, true, true, &COLOR_CYAN);
        PlotThis("Raw Rear Grip", plot_raw_rear_grip, history, 0.0f, 1.2f, true, true, &COLOR_MAGENTA);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Patch Velocities]");
        PlotThis("F-Lat PatchVel", plot_raw_front_lat_patch_vel, history, 0.0f, 20.0f, true, false, &COLOR_CYAN);
        PlotThis("R-Lat PatchVel", plot_raw_rear_lat_patch_vel, history, 0.0f, 20.0f, true, false, &COLOR_MAGENTA);
        PlotThis("F-Long PatchVel", plot_raw_front_long_patch_vel, history, -20.0f, 20.0f, false, true, &COLOR_CYAN);
        PlotThis("R-Long PatchVel", plot_raw_rear_long_patch_vel, history, -20.0f, 20.0f, false, true, &COLOR_MAGENTA);
        ImGui::Columns(1);
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
