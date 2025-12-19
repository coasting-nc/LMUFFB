### Code Changes

I will unlock the ranges for **SoP**, **Oversteer Boost**, and **Rear Align Torque**, and update the **T300 Preset** to use these boosted values automatically.

#### 1. Update `src/GuiLayer.cpp`

```cpp
    ImGui::Separator();
    ImGui::Text("Effects");
    
    ImGui::SliderFloat("Understeer (Grip)", &engine.m_understeer_effect, 0.0f, 50.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Strength of the force drop when grip is lost.\nValues > 1.0 exaggerate the effect.\nHigh values (10-50) create a 'Binary' drop for belt-driven wheels.");

    // CHANGED: Max from 2.0 to 20.0
    ImGui::SliderFloat("SoP (Lateral G)", &engine.m_sop_effect, 0.0f, 20.0f, "%.2f");
    
    // CHANGED: Max from 2.0 to 20.0
    ImGui::SliderFloat("SoP Yaw (Kick)", &engine.m_sop_yaw_gain, 0.0f, 20.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Injects Yaw Acceleration to provide a predictive kick when rotation starts.");
    
    ImGui::SliderFloat("Gyroscopic Damping", &engine.m_gyro_gain, 0.0f, 1.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stabilizes the wheel during drifts by opposing rapid steering movements.\nPrevents oscillations (tank slappers).");
    
    // CHANGED: Max from 1.0 to 20.0
    ImGui::SliderFloat("Oversteer Boost", &engine.m_oversteer_boost, 0.0f, 20.0f, "%.2f");
    
    // CHANGED: Max from 2.0 to 20.0
    ImGui::SliderFloat("Rear Align Torque", &engine.m_rear_align_effect, 0.0f, 20.0f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls rear-end counter-steering feedback.\nProvides a distinct cue during oversteer without affecting base SoP.\nIncrease for stronger rear-end feel (0.0 = Off, 1.0 = Default, 2.0 = Max).");
```

#### 2. Update `src/Config.cpp`

I will update the **"T300"** preset to include these boosted oversteer values, so you have a balanced starting point.

```cpp
    // 2. T300 (User Tuned)
    // Tuned for Thrustmaster T300RS with 100Nm Reference.
    // Boosts effects by ~10x to compensate for the high reference.
    presets.push_back(Preset("T300")
        .SetGain(1.0f)
        .SetShaftGain(1.0f)
        .SetMinForce(0.0f)
        .SetMaxTorque(100.0f)    // High ref to prevent clipping
        .SetInvert(true)
        .SetUndersteer(38.0f)    // Grip Drop
        .SetSoP(5.0f)            // Lateral G (Weight)
        .SetRearAlign(15.0f)     // Counter-Steer Torque (The "Pull")
        .SetOversteer(2.0f)      // Boost when rear slips
        .SetSoPYaw(5.0f)         // Kick on rotation start
        .SetGyro(0.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetBaseMode(0)
    );
```

##  implement a "Dirty State" logic

The GUI currently "lies" to you—it keeps displaying the Preset name even after you have modified the values, which is confusing.

We need to implement a **"Dirty State"** logic:
1.  If you load a preset, display its name.
2.  **The moment you touch any slider**, switch the display to **"Custom"**.

Here is the updated `src/GuiLayer.cpp`. I have refactored `DrawTuningWindow` to use helper functions that automatically detect changes and update the preset status.

### Update `src/GuiLayer.cpp`

Replace the entire file content with this.
*   **Change:** Added helper lambdas (`FloatSetting`, `BoolSetting`, `IntSetting`) that trigger `selected_preset = -1` whenever a value changes.
*   **Change:** The Preset dropdown now displays "Custom" if `selected_preset == -1`.

```cpp
#include "GuiLayer.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>

// Define STB_IMAGE_WRITE_IMPLEMENTATION only once in the project
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4996)
#include "stb_image_write.h"
#pragma warning(pop)
#include <ctime>

#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

// Global DirectX variables
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;
static HWND                     g_hwnd = NULL;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern std::atomic<bool> g_running;
extern std::mutex g_engine_mutex;

#ifndef LMUFFB_VERSION
#define LMUFFB_VERSION "Dev"
#endif

bool GuiLayer::Init() {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"LMUFFB", NULL };
    ::RegisterClassExW(&wc);
    
    std::string ver = LMUFFB_VERSION;
    std::wstring wver(ver.begin(), ver.end());
    std::wstring title = L"LMUFFB v" + wver;

    g_hwnd = ::CreateWindowW(wc.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

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
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            g_running = false;
            return false;
        }
    }
    
    if (g_running == false) return false;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawTuningWindow(engine);
    
    if (m_show_debug_window) {
        DrawDebugWindow(engine);
    }

    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0);
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) || ImGui::IsAnyItemActive();
}

// Screenshot Helper
void SaveScreenshot(const char* filename) {
    if (!g_pSwapChain || !g_pd3dDevice || !g_pd3dDeviceContext) return;

    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) return;

    D3D11_TEXTURE2D_DESC desc;
    pBackBuffer->GetDesc(&desc);
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;

    ID3D11Texture2D* pStagingTexture = nullptr;
    hr = g_pd3dDevice->CreateTexture2D(&desc, NULL, &pStagingTexture);
    if (FAILED(hr)) {
        pBackBuffer->Release();
        return;
    }

    g_pd3dDeviceContext->CopyResource(pStagingTexture, pBackBuffer);

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = g_pd3dDeviceContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        int width = desc.Width;
        int height = desc.Height;
        int channels = 4;
        
        std::vector<unsigned char> image_data(width * height * channels);
        unsigned char* src = (unsigned char*)mapped.pData;
        unsigned char* dst = image_data.data();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int src_index = (y * mapped.RowPitch) + (x * 4);
                int dst_index = (y * width * 4) + (x * 4);
                dst[dst_index + 0] = src[src_index + 0]; // R
                dst[dst_index + 1] = src[src_index + 1]; // G
                dst[dst_index + 2] = src[src_index + 2]; // B
                dst[dst_index + 3] = 255;                // Alpha
            }
        }
        stbi_write_png(filename, width, height, channels, image_data.data(), width * channels);
        g_pd3dDeviceContext->Unmap(pStagingTexture, 0);
    }
    pStagingTexture->Release();
    pBackBuffer->Release();
    std::cout << "[GUI] Screenshot saved to " << filename << std::endl;
}

// Win32 message handler
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
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd) {
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
bool GuiLayer::Init() { return true; }
void GuiLayer::Shutdown() {}
bool GuiLayer::Render(FFBEngine& engine) { return false; }
#endif

// --- CONFIGURABLE PLOT SETTINGS ---
const float PLOT_HISTORY_SEC = 10.0f;
const int PHYSICS_RATE_HZ = 400;
const int PLOT_BUFFER_SIZE = (int)(PLOT_HISTORY_SEC * PHYSICS_RATE_HZ);

// --- Helper: Ring Buffer ---
struct RollingBuffer {
    std::vector<float> data;
    int offset = 0;
    
    RollingBuffer() { data.resize(PLOT_BUFFER_SIZE, 0.0f); }
    
    void Add(float val) {
        data[offset] = val;
        offset = (offset + 1) % data.size();
    }
    
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
    ImGui::Text("%s", label);
    
    char hidden_label[256];
    snprintf(hidden_label, sizeof(hidden_label), "##%s", label);
    
    ImGui::PlotLines(hidden_label, buffer.data.data(), (int)buffer.data.size(), 
                     buffer.offset, NULL, scale_min, scale_max, size);
    
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

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

    ImVec2 box_max = ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y);
    
    draw_list->AddRectFilled(ImVec2(p_min.x - 1, p_min.y), box_max, IM_COL32(0, 0, 0, 150));
    draw_list->AddText(font, font_size, p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
}

// Static buffers
static RollingBuffer plot_total;
static RollingBuffer plot_base;
static RollingBuffer plot_sop;
static RollingBuffer plot_yaw_kick;
static RollingBuffer plot_rear_torque; 
static RollingBuffer plot_gyro_damping;
static RollingBuffer plot_scrub_drag;
static RollingBuffer plot_oversteer;
static RollingBuffer plot_understeer;
static RollingBuffer plot_clipping;
static RollingBuffer plot_road;
static RollingBuffer plot_slide;
static RollingBuffer plot_lockup;
static RollingBuffer plot_spin;
static RollingBuffer plot_bottoming;
static RollingBuffer plot_calc_front_load;
static RollingBuffer plot_calc_rear_load; 
static RollingBuffer plot_calc_front_grip;
static RollingBuffer plot_calc_rear_grip;
static RollingBuffer plot_calc_slip_ratio;
static RollingBuffer plot_calc_slip_angle_smoothed; 
static RollingBuffer plot_calc_rear_slip_angle_smoothed; 
static RollingBuffer plot_calc_rear_lat_force; 
static RollingBuffer plot_raw_steer;
static RollingBuffer plot_raw_input_steering;
static RollingBuffer plot_raw_throttle;    
static RollingBuffer plot_raw_brake;       
static RollingBuffer plot_input_accel;
static RollingBuffer plot_raw_car_speed;   
static RollingBuffer plot_raw_load;        
static RollingBuffer plot_raw_grip;        
static RollingBuffer plot_raw_rear_grip;
static RollingBuffer plot_raw_front_slip_ratio;
static RollingBuffer plot_raw_susp_force;  
static RollingBuffer plot_raw_ride_height; 
static RollingBuffer plot_raw_front_lat_patch_vel; 
static RollingBuffer plot_raw_front_long_patch_vel;
static RollingBuffer plot_raw_rear_lat_patch_vel;
static RollingBuffer plot_raw_rear_long_patch_vel;
static RollingBuffer plot_raw_slip_angle;
static RollingBuffer plot_raw_rear_slip_angle;
static RollingBuffer plot_raw_front_deflection; 

static bool g_warn_load = false;
static bool g_warn_grip = false;
static bool g_warn_dt = false;

bool GuiLayer::m_show_debug_window = false;

void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    ImGui::Begin("FFB Analysis", &m_show_debug_window);
    
    auto snapshots = engine.GetDebugBatch();
    
    for (const auto& snap : snapshots) {
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

    if (g_warn_load || g_warn_grip || g_warn_dt) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("TELEMETRY WARNINGS:");
        if (g_warn_load) ImGui::Text("- Missing Tire Load (Check shared memory)");
        if (g_warn_grip) ImGui::Text("- Missing Grip Data (Ice or Error)");
        if (g_warn_dt) ImGui::Text("- Invalid DeltaTime (Using 400Hz fallback)");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("A. FFB Components (Output)", ImGuiTreeNodeFlags_DefaultOpen)) {
        PlotWithStats("Total Output", plot_total, -1.0f, 1.0f, ImVec2(0, 60), "Final FFB Output (-1.0 to 1.0)");
        
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

    if (ImGui::CollapsingHeader("B. Internal Physics (Brain)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(3, "PhysCols", false);
        
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Loads]");
        
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

    if (ImGui::CollapsingHeader("C. Raw Game Telemetry (Input)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(4, "TelCols", false);
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Driver Input]");
        PlotWithStats("Steering Torque", plot_raw_steer, -30.0f, 30.0f, ImVec2(0, 40));
        PlotWithStats("Steering Input", plot_raw_input_steering, -1.0f, 1.0f, ImVec2(0, 40));
        
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
        
        {
            float current = plot_raw_load.GetCurrent();
            char stats_label[256];
            snprintf(stats_label, sizeof(stats_label), "Raw Front Load | %.0f", current);
            if (g_warn_load) ImGui::TextColored(ImVec4(1,0,0,1), "%s (MISSING)", stats_label);
            else ImGui::Text("%s", stats_label);
            ImGui::PlotLines("##RawLoad", plot_raw_load.data.data(), (int)plot_raw_load.data.size(), plot_raw_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        }
        
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

    // --- PRESET LOGIC WITH DIRTY STATE ---
    static int selected_preset = 0;
    if (Config::presets.empty()) {
        Config::LoadPresets();
    }
    
    // Determine display name: Preset Name or "Custom"
    const char* preview_value = (selected_preset >= 0 && selected_preset < Config::presets.size()) 
                                ? Config::presets[selected_preset].name.c_str() 
                                : "Custom";
    
    if (ImGui::BeginCombo("Load Preset", preview_value)) {
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
    
    // Helper Lambdas to detect changes
    auto FloatSetting = [&](const char* label, float* v, float min, float max, const char* fmt = "%.2f") {
        if (ImGui::SliderFloat(label, v, min, max, fmt)) {
            selected_preset = -1; // Mark as Custom
        }
    };
    
    auto BoolSetting = [&](const char* label, bool* v) {
        if (ImGui::Checkbox(label, v)) {
            selected_preset = -1;
        }
    };
    
    auto IntSetting = [&](const char* label, int* v, const char* const items[], int items_count) {
        if (ImGui::Combo(label, v, items, items_count)) {
            selected_preset = -1;
        }
    };

    FloatSetting("Master Gain", &engine.m_gain, 0.0f, 2.0f);
    FloatSetting("Steering Shaft Gain", &engine.m_steering_shaft_gain, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Attenuates raw game force without affecting telemetry.\nUse this instead of Master Gain if other effects are too weak.");
    FloatSetting("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f");
    
    FloatSetting("Max Torque Ref (Nm)", &engine.m_max_torque_ref, 1.0f, 200.0f, "%.1f Nm");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The torque value that equals 100%% FFB output.\nIncrease this to WEAKEN the FFB (make it lighter).\nFor T300/G29, try 40-100 Nm.");

    if (ImGui::TreeNode("Advanced Tuning")) {
        const char* base_modes[] = { "Native (Physics)", "Synthetic (Constant)", "Muted (Off)" };
        IntSetting("Base Force Mode", &engine.m_base_force_mode, base_modes, IM_ARRAYSIZE(base_modes));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Debug tool to isolate effects.\nNative: Raw physics.\nSynthetic: Constant force to tune Grip drop-off.\nMuted: Zero base force.");

        FloatSetting("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f (1=Raw)");
        FloatSetting("SoP Scale", &engine.m_sop_scale, 0.0f, 200.0f, "%.1f");
        FloatSetting("Load Cap", &engine.m_max_load_factor, 1.0f, 3.0f, "%.1fx");
        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::Text("Effects");
    
    FloatSetting("Understeer (Grip)", &engine.m_understeer_effect, 0.0f, 50.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Strength of the force drop when grip is lost.\nValues > 1.0 exaggerate the effect.\nHigh values (10-50) create a 'Binary' drop for belt-driven wheels.");

    FloatSetting("SoP (Lateral G)", &engine.m_sop_effect, 0.0f, 20.0f);
    FloatSetting("SoP Yaw (Kick)", &engine.m_sop_yaw_gain, 0.0f, 20.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Injects Yaw Acceleration to provide a predictive kick when rotation starts.");
    
    FloatSetting("Gyroscopic Damping", &engine.m_gyro_gain, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stabilizes the wheel during drifts by opposing rapid steering movements.\nPrevents oscillations (tank slappers).");
    
    FloatSetting("Oversteer Boost", &engine.m_oversteer_boost, 0.0f, 20.0f);
    FloatSetting("Rear Align Torque", &engine.m_rear_align_effect, 0.0f, 20.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls rear-end counter-steering feedback.\nProvides a distinct cue during oversteer without affecting base SoP.\nIncrease for stronger rear-end feel (0.0 = Off, 1.0 = Default, 2.0 = Max).");


    ImGui::Separator();
    ImGui::Text("Haptics (Dynamic)");
    BoolSetting("Progressive Lockup", &engine.m_lockup_enabled);
    if (engine.m_lockup_enabled) {
        ImGui::SameLine(); FloatSetting("##Lockup", &engine.m_lockup_gain, 0.0f, 1.0f, "Gain: %.2f");
    }
    
    BoolSetting("Spin Traction Loss", &engine.m_spin_enabled);
    if (engine.m_spin_enabled) {
        ImGui::SameLine(); FloatSetting("##Spin", &engine.m_spin_gain, 0.0f, 1.0f, "Gain: %.2f");
    }
    
    BoolSetting("Use Manual Slip Calc", &engine.m_use_manual_slip);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Calculates Slip Ratio from Wheel Speed vs Car Speed instead of game telemetry.\nUseful if game slip data is broken or zero.");

    ImGui::Separator();
    ImGui::Text("Textures");
    BoolSetting("Slide Rumble", &engine.m_slide_texture_enabled);
    if (engine.m_slide_texture_enabled) {
        ImGui::Indent();
        FloatSetting("Slide Gain", &engine.m_slide_texture_gain, 0.0f, 2.0f);
        ImGui::Unindent();
    }
    BoolSetting("Road Details", &engine.m_road_texture_enabled);
    if (engine.m_road_texture_enabled) {
        ImGui::Indent();
        FloatSetting("Road Gain", &engine.m_road_texture_gain, 0.0f, 5.0f);
        ImGui::Unindent();
    }
    
    FloatSetting("Scrub Drag Gain", &engine.m_scrub_drag_gain, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adds resistance when sliding sideways (tire dragging).");
    
    const char* bottoming_modes[] = { "Method A: Scraping", "Method B: Susp. Spike" };
    IntSetting("Bottoming Logic", &engine.m_bottoming_method, bottoming_modes, IM_ARRAYSIZE(bottoming_modes));

    ImGui::Separator();
    ImGui::Text("Output");
    
    BoolSetting("Invert FFB Signal", &engine.m_invert_force); 
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Check this if the wheel pulls away from center instead of aligning.");

    if (ImGui::Checkbox("Monitor FFB on vJoy (Axis X)", &Config::m_output_ffb_to_vjoy)) {
        // This is a global config, not engine state, but we should still mark custom?
        // Actually, output routing doesn't affect the physics preset usually.
        // But let's mark it custom to be safe.
        selected_preset = -1;
        
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
        selected_preset = -1; // Mark as custom/reset
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
```