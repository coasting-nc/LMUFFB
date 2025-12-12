#include "GuiLayer.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include <iostream>
#include <vector>
#include <mutex>

#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

// Global DirectX variables (Simplified for brevity, usually managed in a separate backend class)
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;
static HWND                     g_hwnd = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// External linkage to FFB loop status
extern std::atomic<bool> g_running;
extern std::mutex g_engine_mutex;

// Macro stringification helper
#define XSTR(x) STR(x)
#define STR(x) #x

// If VERSION is not defined via CMake, default
#ifndef LMUFFB_VERSION
#define LMUFFB_VERSION "Dev"
#endif

bool GuiLayer::Init() {
    // Create Application Window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"LMUFFB", NULL };
    ::RegisterClassExW(&wc);
    
    // Construct Title with Version
    // We need wide string for CreateWindowW. 
    // Simplified conversion for version string (assumes ASCII version)
    std::string ver = LMUFFB_VERSION;
    std::wstring wver(ver.begin(), ver.end());
    std::wstring title = L"LMUFFB v" + wver;

    g_hwnd = ::CreateWindowW(wc.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    // Show the window
    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    return true;
}

void GuiLayer::Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(L"LMUFFB", GetModuleHandle(NULL));
}

void* GuiLayer::GetWindowHandle() {
    return (void*)g_hwnd;
}

bool GuiLayer::Render(FFBEngine& engine) {
    // Handle Windows messages
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            g_running = false;
            return false;
        }
    }
    
    // If minimized, sleep to save CPU (Lazy Rendering)
    // Note: In a real app we'd check IsIconic(g_hwnd) outside this logic or return a 'should_sleep' flag
    if (g_running == false) return false;

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Draw Tuning Window
    DrawTuningWindow(engine);
    
    // Draw Debug Window (if enabled)
    if (m_show_debug_window) {
        DrawDebugWindow(engine);
    }

    // Rendering
    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0); // Present with vsync

    // Return focus state for lazy rendering optimization
    // Typically, if ImGui::IsAnyItemActive() is true, we want fast updates
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) || ImGui::IsAnyItemActive();
}

void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    // LOCK MUTEX to prevent race condition with FFB Thread
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    // Show Version in title bar or top text
    std::string title = std::string("LMUFFB v") + LMUFFB_VERSION + " - FFB Configuration";
    ImGui::Begin(title.c_str());

    // --- CONNECTION STATUS ---
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
    
    // Device Selection
    static std::vector<DeviceInfo> devices;
    static int selected_device_idx = -1;
    
    // Scan button (or auto scan once)
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
    
    // NEW: Unbind Device Button
    ImGui::SameLine();
    if (ImGui::Button("Unbind Device")) {
        DirectInputFFB::Get().ReleaseDevice();
        selected_device_idx = -1;
    }

    ImGui::Separator();

    // --- PRESETS ---
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
    ImGui::SliderFloat("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f");
    // New Max Torque Ref Slider (v0.4.4)
    ImGui::SliderFloat("Max Torque Ref (Nm)", &engine.m_max_torque_ref, 10.0f, 100.0f, "%.1f Nm");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The torque value that equals 100%% FFB output.\nIncrease this if FFB is too strong at Gain 1.0.\nTypical values: 20-40 Nm.");

    if (ImGui::TreeNode("Advanced Tuning")) {
        ImGui::SliderFloat("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f (1=Raw)");
        ImGui::SliderFloat("SoP Scale", &engine.m_sop_scale, 100.0f, 5000.0f, "%.0f");
        ImGui::SliderFloat("Load Cap", &engine.m_max_load_factor, 1.0f, 3.0f, "%.1fx");
        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::Text("Effects");
    ImGui::SliderFloat("Understeer (Grip)", &engine.m_understeer_effect, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("SoP (Lateral G)", &engine.m_sop_effect, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Oversteer Boost", &engine.m_oversteer_boost, 0.0f, 1.0f, "%.2f");

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
    
    // v0.4.5: Manual Slip Calculation Toggle
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
    
    // v0.4.5: Scrub Drag Effect
    ImGui::SliderFloat("Scrub Drag Gain", &engine.m_scrub_drag_gain, 0.0f, 1.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adds resistance when sliding sideways (tire dragging).");
    
    // v0.4.5: Bottoming Method
    const char* bottoming_modes[] = { "Method A: Scraping", "Method B: Susp. Spike" };
    ImGui::Combo("Bottoming Logic", &engine.m_bottoming_method, bottoming_modes, IM_ARRAYSIZE(bottoming_modes));

    ImGui::Separator();
    ImGui::Text("Output");
    
    // Invert Force (v0.4.4)
    ImGui::Checkbox("Invert FFB Signal", &engine.m_invert_force); 
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Check this if the wheel pulls away from center instead of aligning.");

    // vJoy Monitoring (Safety critical)
    if (ImGui::Checkbox("Monitor FFB on vJoy (Axis X)", &Config::m_output_ffb_to_vjoy)) {
        // Warn user if enabling
        if (Config::m_output_ffb_to_vjoy) {
            MessageBoxA(NULL, "WARNING: Enabling this will output the FFB signal to vJoy Axis X.\n\n"
                              "If you have bound Game Steering to vJoy Axis X, this will cause a Feedback Loop (Wheel Spinning).\n"
                              "Only enable this if you are NOT using vJoy Axis X for steering.", 
                              "Safety Warning", MB_ICONWARNING | MB_OK);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Outputs calculated force to vJoy Axis X for visual monitoring in vJoy Monitor.\nDISABLE if binding steering to vJoy Axis X!");

    // Visualize Clipping (this requires the calculated force from the engine passed back, 
    // or we just show the static gain for now. A real app needs a shared state for 'last_output_force')
    // ImGui::ProgressBar((float)engine.last_force, ImVec2(0.0f, 0.0f)); 
    ImGui::Text("Clipping Visualization Placeholder");

    ImGui::Separator();
    if (ImGui::Button("Save Configuration")) {
        Config::Save(engine);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Show Troubleshooting Graphs", &m_show_debug_window)) {
        // Just toggles window
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset Defaults")) {
        // Reset Logic (Updated v0.3.13)
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

    ImGui::End();
}

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

// Helper functions for D3D (boilerplate)
bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

#else
// Stub Implementation for Headless Builds
bool GuiLayer::Init() { 
    std::cout << "[GUI] Disabled (Headless Mode)" << std::endl; 
    return true; 
}
void GuiLayer::Shutdown() {}
bool GuiLayer::Render(FFBEngine& engine) { return false; } // Always lazy
#endif

// --- CONFIGURABLE PLOT SETTINGS ---
const float PLOT_HISTORY_SEC = 10.0f;   // 10 Seconds History
const int PHYSICS_RATE_HZ = 400;        // Fixed update rate
const int PLOT_BUFFER_SIZE = (int)(PLOT_HISTORY_SEC * PHYSICS_RATE_HZ); // 4000 points

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
};

// --- Header A: FFB Components ---
static RollingBuffer plot_total;
static RollingBuffer plot_base;
static RollingBuffer plot_sop;
static RollingBuffer plot_understeer;
static RollingBuffer plot_oversteer;
static RollingBuffer plot_rear_torque; // New v0.4.7
static RollingBuffer plot_scrub_drag;  // New v0.4.7
static RollingBuffer plot_road;
static RollingBuffer plot_slide;
static RollingBuffer plot_lockup;
static RollingBuffer plot_spin;
static RollingBuffer plot_bottoming;
static RollingBuffer plot_clipping;

// --- Header B: Internal Physics ---
static RollingBuffer plot_calc_front_load; // New v0.4.7
static RollingBuffer plot_calc_front_grip; // New v0.4.7
static RollingBuffer plot_calc_slip_ratio; // New v0.4.7
static RollingBuffer plot_calc_slip_angle; 

// --- Header C: Raw Game Telemetry ---
static RollingBuffer plot_raw_steer;
static RollingBuffer plot_raw_load;        // New v0.4.7
static RollingBuffer plot_raw_grip;        // New v0.4.7
static RollingBuffer plot_raw_susp_force;  // New v0.4.7
static RollingBuffer plot_raw_ride_height; // New v0.4.7
static RollingBuffer plot_raw_car_speed;   // New v0.4.7
static RollingBuffer plot_raw_throttle;    // New v0.4.7
static RollingBuffer plot_raw_brake;       // New v0.4.7
static RollingBuffer plot_input_accel;
static RollingBuffer plot_input_patch_vel;
static RollingBuffer plot_input_vert_deflection;

// State for Warnings
static bool g_warn_load = false;
static bool g_warn_grip = false;
static bool g_warn_dt = false;

// Toggle State
bool GuiLayer::m_show_debug_window = false;

void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    ImGui::Begin("FFB Analysis", &m_show_debug_window);
    
    // Retrieve latest snapshots from the FFB thread
    auto snapshots = engine.GetDebugBatch();
    
    // Update buffers with the latest snapshot (if available)
    // Loop through ALL snapshots to avoid aliasing
    for (const auto& snap : snapshots) {
        // --- Header A: FFB Components ---
        plot_total.Add(snap.total_output);
        plot_base.Add(snap.base_force);
        plot_sop.Add(snap.sop_force);
        plot_understeer.Add(snap.understeer_drop);
        plot_oversteer.Add(snap.oversteer_boost);
        plot_rear_torque.Add(snap.ffb_rear_torque);
        plot_scrub_drag.Add(snap.ffb_scrub_drag);
        plot_road.Add(snap.texture_road);
        plot_slide.Add(snap.texture_slide);
        plot_lockup.Add(snap.texture_lockup);
        plot_spin.Add(snap.texture_spin);
        plot_bottoming.Add(snap.texture_bottoming);
        plot_clipping.Add(snap.clipping);

        // --- Header B: Internal Physics ---
        plot_calc_front_load.Add(snap.calc_front_load);
        plot_calc_front_grip.Add(snap.calc_front_grip);
        plot_calc_slip_ratio.Add(snap.calc_front_slip_ratio);
        plot_calc_slip_angle.Add(snap.slip_angle);

        // --- Header C: Raw Telemetry ---
        plot_raw_steer.Add(snap.steer_force);
        plot_raw_load.Add(snap.raw_front_tire_load);
        plot_raw_grip.Add(snap.raw_front_grip_fract);
        plot_raw_susp_force.Add(snap.raw_front_susp_force);
        plot_raw_ride_height.Add(snap.raw_front_ride_height);
        plot_raw_car_speed.Add(snap.raw_car_speed);
        plot_raw_throttle.Add(snap.raw_input_throttle);
        plot_raw_brake.Add(snap.raw_input_brake);
        plot_input_accel.Add(snap.accel_x);
        plot_input_patch_vel.Add(snap.patch_vel);
        plot_input_vert_deflection.Add(snap.deflection);

        // Update Warning Flags (Sticky-ish for display)
        g_warn_load = snap.warn_load;
        g_warn_grip = snap.warn_grip;
        g_warn_dt = snap.warn_dt;
    }

    // --- Draw Warnings ---
    if (g_warn_load || g_warn_grip || g_warn_dt) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("TELEMETRY WARNINGS:");
        if (g_warn_load) ImGui::Text("- Missing Tire Load (Check shared memory)");
        if (g_warn_grip) ImGui::Text("- Missing Grip Data (Ice or Error)");
        if (g_warn_dt) ImGui::Text("- Invalid DeltaTime (Using 400Hz fallback)");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // --- Header A: FFB Components (The Output) ---
    if (ImGui::CollapsingHeader("A. FFB Components (Output Stack)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Total Output");
        ImGui::PlotLines("##Total", plot_total.data.data(), (int)plot_total.data.size(), plot_total.offset, "Total", -1.0f, 1.0f, ImVec2(0, 60));
        
        ImGui::Columns(2, "FFBCols", false);
        ImGui::Text("Base Torque (Nm)"); ImGui::PlotLines("##Base", plot_base.data.data(), (int)plot_base.data.size(), plot_base.offset, NULL, -30.0f, 30.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("SoP (Lat G)"); ImGui::PlotLines("##SoP", plot_sop.data.data(), (int)plot_sop.data.size(), plot_sop.offset, NULL, -1000.0f, 1000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Understeer Cut"); ImGui::PlotLines("##Under", plot_understeer.data.data(), (int)plot_understeer.data.size(), plot_understeer.offset, NULL, -2000.0f, 2000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Oversteer Boost"); ImGui::PlotLines("##Over", plot_oversteer.data.data(), (int)plot_oversteer.data.size(), plot_oversteer.offset, NULL, -500.0f, 500.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Rear Align Torque"); ImGui::PlotLines("##RearT", plot_rear_torque.data.data(), (int)plot_rear_torque.data.size(), plot_rear_torque.offset, NULL, -500.0f, 500.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Scrub Drag Force"); ImGui::PlotLines("##Drag", plot_scrub_drag.data.data(), (int)plot_scrub_drag.data.size(), plot_scrub_drag.offset, NULL, -500.0f, 500.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Road Texture"); ImGui::PlotLines("##Road", plot_road.data.data(), (int)plot_road.data.size(), plot_road.offset, NULL, -1000.0f, 1000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Slide Texture"); ImGui::PlotLines("##Slide", plot_slide.data.data(), (int)plot_slide.data.size(), plot_slide.offset, NULL, -500.0f, 500.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Lockup Vib"); ImGui::PlotLines("##Lock", plot_lockup.data.data(), (int)plot_lockup.data.size(), plot_lockup.offset, NULL, -500.0f, 500.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Spin Vib"); ImGui::PlotLines("##Spin", plot_spin.data.data(), (int)plot_spin.data.size(), plot_spin.offset, NULL, -500.0f, 500.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Bottoming"); ImGui::PlotLines("##Bot", plot_bottoming.data.data(), (int)plot_bottoming.data.size(), plot_bottoming.offset, NULL, -1000.0f, 1000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Clipping"); ImGui::PlotLines("##Clip", plot_clipping.data.data(), (int)plot_clipping.data.size(), plot_clipping.offset, NULL, 0.0f, 1.1f, ImVec2(0, 40));
        ImGui::Columns(1);
    }

    // --- Header B: Internal Physics Engine (The Brain) ---
    if (ImGui::CollapsingHeader("B. Internal Physics (Brain)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(2, "PhysCols", false);
        
        ImGui::Text("Calc Front Load (N)");
        ImGui::PlotLines("##CalcLoad", plot_calc_front_load.data.data(), (int)plot_calc_front_load.data.size(), plot_calc_front_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Calc Front Grip");
        ImGui::PlotLines("##CalcGrip", plot_calc_front_grip.data.data(), (int)plot_calc_front_grip.data.size(), plot_calc_front_grip.offset, NULL, 0.0f, 1.2f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Calc Slip Ratio");
        ImGui::PlotLines("##CalcSlip", plot_calc_slip_ratio.data.data(), (int)plot_calc_slip_ratio.data.size(), plot_calc_slip_ratio.offset, NULL, -1.0f, 1.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Smoothed Slip Angle");
        ImGui::PlotLines("##SlipA", plot_calc_slip_angle.data.data(), (int)plot_calc_slip_angle.data.size(), plot_calc_slip_angle.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::Columns(1);
    }

    // --- Header C: Raw Game Telemetry Inspector (The Input) ---
    if (ImGui::CollapsingHeader("C. Raw Game Telemetry (Input)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(2, "TelCols", false);
        
        ImGui::Text("Steering Torque (Nm)"); 
        ImGui::PlotLines("##StForce", plot_raw_steer.data.data(), (int)plot_raw_steer.data.size(), plot_raw_steer.offset, NULL, -30.0f, 30.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        // Highlight Load if warning
        if (g_warn_load) ImGui::TextColored(ImVec4(1,0,0,1), "Raw Front Load (MISSING)");
        else ImGui::Text("Raw Front Load (N)");
        ImGui::PlotLines("##RawLoad", plot_raw_load.data.data(), (int)plot_raw_load.data.size(), plot_raw_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        // Highlight Grip if warning
        if (g_warn_grip) ImGui::TextColored(ImVec4(1,0,0,1), "Raw Front Grip (MISSING)");
        else ImGui::Text("Raw Front Grip");
        ImGui::PlotLines("##RawGrip", plot_raw_grip.data.data(), (int)plot_raw_grip.data.size(), plot_raw_grip.offset, NULL, 0.0f, 1.2f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Raw Front Susp. Force"); 
        ImGui::PlotLines("##Susp", plot_raw_susp_force.data.data(), (int)plot_raw_susp_force.data.size(), plot_raw_susp_force.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Raw Front Ride Height"); 
        ImGui::PlotLines("##RH", plot_raw_ride_height.data.data(), (int)plot_raw_ride_height.data.size(), plot_raw_ride_height.offset, NULL, 0.0f, 0.1f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Car Speed (m/s)"); 
        ImGui::PlotLines("##Speed", plot_raw_car_speed.data.data(), (int)plot_raw_car_speed.data.size(), plot_raw_car_speed.offset, NULL, 0.0f, 100.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Throttle Input"); 
        ImGui::PlotLines("##Thr", plot_raw_throttle.data.data(), (int)plot_raw_throttle.data.size(), plot_raw_throttle.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Brake Input"); 
        ImGui::PlotLines("##Brk", plot_raw_brake.data.data(), (int)plot_raw_brake.data.size(), plot_raw_brake.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Local Accel X"); 
        ImGui::PlotLines("##LatG", plot_input_accel.data.data(), (int)plot_input_accel.data.size(), plot_input_accel.offset, NULL, -20.0f, 20.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Avg Lat PatchVel"); 
        ImGui::PlotLines("##PatchV", plot_input_patch_vel.data.data(), (int)plot_input_patch_vel.data.size(), plot_input_patch_vel.offset, NULL, 0.0f, 20.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        
        ImGui::Text("Avg Deflection"); 
        ImGui::PlotLines("##Defl", plot_input_vert_deflection.data.data(), (int)plot_input_vert_deflection.data.size(), plot_input_vert_deflection.offset, NULL, 0.0f, 0.1f, ImVec2(0, 40));
        ImGui::Columns(1);
    }

    ImGui::End();
}
