#include "GuiLayer.h"
#include "Config.h"
#include "DirectInputFFB.h"
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

    ImGui::Separator();
    
    ImGui::SliderFloat("Master Gain", &engine.m_gain, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f");

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

    ImGui::Separator();
    ImGui::Text("Output");
    
    // vJoy Monitoring (Safety critical)
    if (ImGui::Checkbox("Enable vJoy Output (Monitor)", &Config::m_output_ffb_to_vjoy)) {
        // Warn user if enabling
        if (Config::m_output_ffb_to_vjoy) {
            MessageBoxA(NULL, "WARNING: Enabling this will output the FFB signal to vJoy Axis X.\n\n"
                              "If you have bound Game Steering to vJoy Axis X, this will cause a Feedback Loop (Wheel Spinning).\n"
                              "Only enable this if you are NOT using vJoy Axis X for steering.", 
                              "Safety Warning", MB_ICONWARNING | MB_OK);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Acquires the vJoy device and outputs FFB force to Axis X.\nREQUIRED if you want to see forces in vJoy Monitor.\nDISABLE if you use Joystick Gremlin (Gremlin needs exclusive access).");

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

// --- Helper: Ring Buffer for PlotLines ---
struct RollingBuffer {
    std::vector<float> data;
    int offset = 0;
    
    RollingBuffer(int size = 2000) {
        data.resize(size, 0.0f);
    }
    
    void Add(float val) {
        data[offset] = val;
        offset = (offset + 1) % data.size();
    }
};

// Static buffers for debug plots (FFB)
static RollingBuffer plot_total(1000);
static RollingBuffer plot_base(1000);
static RollingBuffer plot_sop(1000);
static RollingBuffer plot_understeer(1000);
static RollingBuffer plot_oversteer(1000);
static RollingBuffer plot_road(1000);
static RollingBuffer plot_slide(1000);
static RollingBuffer plot_lockup(1000);
static RollingBuffer plot_spin(1000);
static RollingBuffer plot_bottoming(1000);
static RollingBuffer plot_clipping(1000);

// Static buffers for debug plots (Telemetry)
static RollingBuffer plot_input_steer(1000); // SteeringArmForce
static RollingBuffer plot_input_accel(1000); // LocalAccel.x
static RollingBuffer plot_input_load(1000); // TireLoad
static RollingBuffer plot_input_grip(1000); // GripFract
static RollingBuffer plot_input_slip_ratio(1000); // SlipRatio
static RollingBuffer plot_input_slip_angle(1000); // SlipAngle
static RollingBuffer plot_input_patch_vel(1000); // PatchVel
static RollingBuffer plot_input_vert_deflection(1000); // Deflection

// Toggle State
bool GuiLayer::m_show_debug_window = false;

void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    ImGui::Begin("FFB Analysis", &m_show_debug_window);
    
    // Update Buffers (Ideally this should be done in the FFB thread and passed via a queue,
    // but sampling at 60Hz from the latest state is "good enough" for visual inspection)
    // Note: This misses high-freq spikes between frames, but shows trends.
    // For Phase 3 (Logging), we need proper buffering.
    
    // --- Update Buffers ---
    // FFB
    plot_total.Add(engine.m_last_debug.total_output);
    plot_base.Add(engine.m_last_debug.base_force);
    plot_sop.Add(engine.m_last_debug.sop_force);
    plot_understeer.Add(engine.m_last_debug.understeer_drop);
    plot_oversteer.Add(engine.m_last_debug.oversteer_boost);
    plot_road.Add(engine.m_last_debug.texture_road);
    plot_slide.Add(engine.m_last_debug.texture_slide);
    plot_lockup.Add(engine.m_last_debug.texture_lockup);
    plot_spin.Add(engine.m_last_debug.texture_spin);
    plot_bottoming.Add(engine.m_last_debug.texture_bottoming);
    plot_clipping.Add(engine.m_last_debug.clipping);

    // Telemetry
    plot_input_steer.Add((float)engine.m_last_telemetry.mSteeringArmForce);
    plot_input_accel.Add((float)engine.m_last_telemetry.mLocalAccel.x);
    // Use MAX of FL/FR or Avg? Usually max is interesting for load, avg for others.
    // Let's use AVG for simplicity in single line.
    const rF2Wheel& fl = engine.m_last_telemetry.mWheels[0];
    const rF2Wheel& fr = engine.m_last_telemetry.mWheels[1];
    
    plot_input_load.Add((float)((fl.mTireLoad + fr.mTireLoad) / 2.0));
    plot_input_grip.Add((float)((fl.mGripFract + fr.mGripFract) / 2.0));
    plot_input_slip_ratio.Add((float)((fl.mSlipRatio + fr.mSlipRatio) / 2.0));
    plot_input_slip_angle.Add((float)((std::abs(fl.mSlipAngle) + std::abs(fr.mSlipAngle)) / 2.0));
    plot_input_patch_vel.Add((float)((std::abs(fl.mLateralPatchVel) + std::abs(fr.mLateralPatchVel)) / 2.0));
    plot_input_vert_deflection.Add((float)((fl.mVerticalTireDeflection + fr.mVerticalTireDeflection) / 2.0));
    
    // --- Draw UI ---
    if (ImGui::CollapsingHeader("FFB Components (Stack)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Total Output");
        ImGui::PlotLines("##Total", plot_total.data.data(), (int)plot_total.data.size(), plot_total.offset, "Total", -1.0f, 1.0f, ImVec2(0, 60));
        
        ImGui::Columns(2, "FFBCols", false);
        ImGui::Text("Base Force"); ImGui::PlotLines("##Base", plot_base.data.data(), (int)plot_base.data.size(), plot_base.offset, NULL, -4000.0f, 4000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("SoP (Lat G)"); ImGui::PlotLines("##SoP", plot_sop.data.data(), (int)plot_sop.data.size(), plot_sop.offset, NULL, -1000.0f, 1000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Understeer Cut"); ImGui::PlotLines("##Under", plot_understeer.data.data(), (int)plot_understeer.data.size(), plot_understeer.offset, NULL, -2000.0f, 2000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Oversteer Boost"); ImGui::PlotLines("##Over", plot_oversteer.data.data(), (int)plot_oversteer.data.size(), plot_oversteer.offset, NULL, -500.0f, 500.0f, ImVec2(0, 40));
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

    if (ImGui::CollapsingHeader("Telemetry Inspector (Raw)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(2, "TelCols", false);
        ImGui::Text("Steering Force"); ImGui::PlotLines("##StForce", plot_input_steer.data.data(), (int)plot_input_steer.data.size(), plot_input_steer.offset, NULL, -5000.0f, 5000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Local Accel X"); ImGui::PlotLines("##LatG", plot_input_accel.data.data(), (int)plot_input_accel.data.size(), plot_input_accel.offset, NULL, -20.0f, 20.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Avg Tire Load"); ImGui::PlotLines("##Load", plot_input_load.data.data(), (int)plot_input_load.data.size(), plot_input_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Avg Grip Fract"); ImGui::PlotLines("##Grip", plot_input_grip.data.data(), (int)plot_input_grip.data.size(), plot_input_grip.offset, NULL, 0.0f, 1.2f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Avg Slip Ratio"); ImGui::PlotLines("##SlipR", plot_input_slip_ratio.data.data(), (int)plot_input_slip_ratio.data.size(), plot_input_slip_ratio.offset, NULL, -1.0f, 1.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Avg Slip Angle"); ImGui::PlotLines("##SlipA", plot_input_slip_angle.data.data(), (int)plot_input_slip_angle.data.size(), plot_input_slip_angle.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Avg Lat PatchVel"); ImGui::PlotLines("##PatchV", plot_input_patch_vel.data.data(), (int)plot_input_patch_vel.data.size(), plot_input_patch_vel.offset, NULL, 0.0f, 20.0f, ImVec2(0, 40));
        ImGui::NextColumn();
        ImGui::Text("Avg Deflection"); ImGui::PlotLines("##Defl", plot_input_vert_deflection.data.data(), (int)plot_input_vert_deflection.data.size(), plot_input_vert_deflection.offset, NULL, 0.0f, 0.1f, ImVec2(0, 40));
        ImGui::Columns(1);
    }

    ImGui::End();
}
