#include "GuiLayer.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>

// Define STB_IMAGE_WRITE_IMPLEMENTATION only once in the project (here is fine)
#define STB_IMAGE_WRITE_IMPLEMENTATION
// Suppress deprecation warning for sprintf in stb_image_write.h (third-party library)
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

// Screenshot Helper (DirectX 11)
void SaveScreenshot(const char* filename) {
    if (!g_pSwapChain || !g_pd3dDevice || !g_pd3dDeviceContext) return;

    // 1. Get the Back Buffer
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) return;

    // 2. Create a Staging Texture (CPU Readable)
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

    // 3. Copy GPU -> CPU
    g_pd3dDeviceContext->CopyResource(pStagingTexture, pBackBuffer);

    // 4. Map the data to read it
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = g_pd3dDeviceContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        // 5. Handle Format (DX11 is usually BGRA, PNG needs RGBA)
        int width = desc.Width;
        int height = desc.Height;
        int channels = 4;
        
        // Allocate buffer for the image
        std::vector<unsigned char> image_data(width * height * channels);
        unsigned char* src = (unsigned char*)mapped.pData;
        unsigned char* dst = image_data.data();

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Calculate positions
                int src_index = (y * mapped.RowPitch) + (x * 4);
                int dst_index = (y * width * 4) + (x * 4);

                // Copy directly - DirectX buffer appears to already be in RGBA format
                dst[dst_index + 0] = src[src_index + 0]; // R
                dst[dst_index + 1] = src[src_index + 1]; // G
                dst[dst_index + 2] = src[src_index + 2]; // B
                dst[dst_index + 3] = 255;                // Alpha (Force Opaque)
            }
        }

        // 6. Save to PNG using STB
        stbi_write_png(filename, width, height, channels, image_data.data(), width * channels);

        g_pd3dDeviceContext->Unmap(pStagingTexture, 0);
    }

    // Cleanup
    pStagingTexture->Release();
    pBackBuffer->Release();
    
    std::cout << "[GUI] Screenshot saved to " << filename << std::endl;
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

    // Display Acquisition Mode
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

    // --- PRESETS WITH DIRTY STATE TRACKING ---
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
    
    // NEW: Save As Preset UI
    static char new_preset_name[64] = "";
    ImGui::InputText("##NewPresetName", new_preset_name, 64);
    ImGui::SameLine();
    if (ImGui::Button("Save as New Preset")) {
        if (strlen(new_preset_name) > 0) {
            Config::AddUserPreset(std::string(new_preset_name), engine);
            // Auto-select the new preset
            for (int i = 0; i < Config::presets.size(); i++) {
                if (Config::presets[i].name == std::string(new_preset_name)) {
                    selected_preset = i;
                    break;
                }
            }
            // Clear buffer
            new_preset_name[0] = '\0';
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save current settings as a new custom preset.");

    ImGui::Separator();
    
    // Helper Lambdas to detect changes and mark as "Custom"
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
    // New Max Torque Ref Slider (v0.4.4)
    FloatSetting("Max Torque Ref (Nm)", &engine.m_max_torque_ref, 1.0f, 200.0f, "%.1f Nm");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The torque value that equals 100%% FFB output.\nIncrease this to WEAKEN the FFB (make it lighter).\nFor T300/G29, try 40-100 Nm.");

    if (ImGui::TreeNode("Advanced Tuning")) {
        // Base Force Mode (v0.4.13)
        const char* base_modes[] = { "Native (Physics)", "Synthetic (Constant)", "Muted (Off)" };
        IntSetting("Base Force Mode", &engine.m_base_force_mode, base_modes, IM_ARRAYSIZE(base_modes));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Debug tool to isolate effects.\nNative: Raw physics.\nSynthetic: Constant force to tune Grip drop-off.\nMuted: Zero base force.");

        FloatSetting("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f (1=Raw)");
        FloatSetting("SoP Scale", &engine.m_sop_scale, 0.0f, 20.0f, "%.1f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scales Lateral G to Nm.\n5.0 = Balanced (10Nm at 2G).\n20.0 = Heavy (40Nm at 2G).");
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
    
    // v0.4.5: Manual Slip Calculation Toggle
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
    
    // v0.4.5: Scrub Drag Effect
    FloatSetting("Scrub Drag Gain", &engine.m_scrub_drag_gain, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adds resistance when sliding sideways (tire dragging).");
    
    // v0.4.5: Bottoming Method
    const char* bottoming_modes[] = { "Method A: Scraping", "Method B: Susp. Spike" };
    IntSetting("Bottoming Logic", &engine.m_bottoming_method, bottoming_modes, IM_ARRAYSIZE(bottoming_modes));

    ImGui::Separator();
    ImGui::Text("Output");
    
    // Invert Force (v0.4.4)
    BoolSetting("Invert FFB Signal", &engine.m_invert_force); 
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Check this if the wheel pulls away from center instead of aligning.");

    // vJoy Monitoring (Safety critical)
    if (ImGui::Checkbox("Monitor FFB on vJoy (Axis X)", &Config::m_output_ffb_to_vjoy)) {
        selected_preset = -1; // Mark as custom
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
    if (ImGui::Button("Reset Defaults")) {
        // Apply the 'Default' preset (index 0) which uses modern values from Config.h
        Config::ApplyPreset(0, engine);
        selected_preset = 0; // Select it in the dropdown
    }
    
    ImGui::Separator();
    if (ImGui::Checkbox("Show Troubleshooting Graphs", &m_show_debug_window)) {
        // Just toggles window
    }
    // Screenshot Button
    ImGui::SameLine();
    if (ImGui::Button("Save Screenshot")) {
        // Generate a timestamped filename
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "screenshot_%Y-%m-%d_%H-%M-%S.png", &tstruct);
        
        SaveScreenshot(buf);
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
    
    // Get the most recent value (current)
    float GetCurrent() const {
        if (data.empty()) return 0.0f;
        // Most recent value is at (offset - 1), wrapping around
        size_t idx = (offset - 1 + static_cast<int>(data.size())) % data.size();
        return data[idx];
    }
    
    // Get minimum value in buffer (optional, for diagnostics)
    float GetMin() const {
        if (data.empty()) return 0.0f;
        return *std::min_element(data.begin(), data.end());
    }
    
    // Get maximum value in buffer (optional, for diagnostics)
    float GetMax() const {
        if (data.empty()) return 0.0f;
        return *std::max_element(data.begin(), data.end());
    }
};

// Helper function to plot with numerical readouts
// Displays: [Title]
// Overlay:  Cur: X.XXXX Min: Y.YYY Max: Z.ZZZ (Small print)
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
    
    // 3. Handle Tooltip
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    // 4. Draw Stats Overlay (Small Legend)
    float current = buffer.GetCurrent();
    float min_val = buffer.GetMin();
    float max_val = buffer.GetMax();
    
    char stats_overlay[128];
    snprintf(stats_overlay, sizeof(stats_overlay), "Cur:%.4f Min:%.3f Max:%.3f", 
             current, min_val, max_val);
    
    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
    float plot_width = p_max.x - p_min.x;
    
    // Padding
    p_min.x += 2;
    p_min.y += 2;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Use current font but scaled down (Small Print)
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize(); // Full resolution
    
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
    
    // Adaptive Formatting: If text is too wide, switch to compact mode
    if (text_size.x > plot_width - 4) {
         // Compact: 0.0000 [0.000, 0.000]
         snprintf(stats_overlay, sizeof(stats_overlay), "%.4f [%.3f, %.3f]", current, min_val, max_val);
         text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         
         // If still too wide, just show current value
         if (text_size.x > plot_width - 4) {
             snprintf(stats_overlay, sizeof(stats_overlay), "Val: %.4f", current);
             text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         }
    }

    ImVec2 box_max = ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y);
    
    // Semi-transparent background (Alpha 90/255 approx 35%)
    draw_list->AddRectFilled(ImVec2(p_min.x - 1, p_min.y), box_max, IM_COL32(0, 0, 0, 90));
    
    // Draw Text with scaled font
    draw_list->AddText(font, font_size, p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
}

// --- Header A: FFB Components (Output) ---
static RollingBuffer plot_total;
static RollingBuffer plot_base;
static RollingBuffer plot_sop;
static RollingBuffer plot_yaw_kick; // New v0.4.15
static RollingBuffer plot_rear_torque; 
static RollingBuffer plot_gyro_damping; // New v0.4.17
static RollingBuffer plot_scrub_drag;
static RollingBuffer plot_oversteer;
static RollingBuffer plot_understeer;
static RollingBuffer plot_clipping;
static RollingBuffer plot_road;
static RollingBuffer plot_slide;
static RollingBuffer plot_lockup;
static RollingBuffer plot_spin;
static RollingBuffer plot_bottoming;

// --- Header B: Internal Physics (Brain) ---
static RollingBuffer plot_calc_front_load;
static RollingBuffer plot_calc_rear_load; 
static RollingBuffer plot_calc_front_grip;
static RollingBuffer plot_calc_rear_grip;
static RollingBuffer plot_calc_slip_ratio;
static RollingBuffer plot_calc_slip_angle_smoothed; 
static RollingBuffer plot_calc_rear_slip_angle_smoothed; 
// Moved here from Header C
static RollingBuffer plot_calc_rear_lat_force; 

// --- Header C: Raw Game Telemetry (Input) ---
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

// Extras
static RollingBuffer plot_raw_slip_angle; // Kept but grouped appropriately
static RollingBuffer plot_raw_rear_slip_angle;
static RollingBuffer plot_raw_front_deflection; 

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
        plot_yaw_kick.Add(snap.ffb_yaw_kick);
        plot_rear_torque.Add(snap.ffb_rear_torque);
        plot_gyro_damping.Add(snap.ffb_gyro_damping); // Add to plot
        plot_scrub_drag.Add(snap.ffb_scrub_drag);
        
        plot_oversteer.Add(snap.oversteer_boost);
        plot_understeer.Add(snap.understeer_drop);
        plot_clipping.Add(snap.clipping);
        
        plot_road.Add(snap.texture_road);
        plot_slide.Add(snap.texture_slide);
        plot_lockup.Add(snap.texture_lockup);
        plot_spin.Add(snap.texture_spin);
        plot_bottoming.Add(snap.texture_bottoming);

        // --- Header B: Internal Physics ---
        plot_calc_front_load.Add(snap.calc_front_load);
        plot_calc_rear_load.Add(snap.calc_rear_load); 
        plot_calc_front_grip.Add(snap.calc_front_grip);
        plot_calc_rear_grip.Add(snap.calc_rear_grip);
        plot_calc_slip_ratio.Add(snap.calc_front_slip_ratio);
        plot_calc_slip_angle_smoothed.Add(snap.calc_front_slip_angle_smoothed);
        plot_calc_rear_slip_angle_smoothed.Add(snap.calc_rear_slip_angle_smoothed);
        plot_calc_rear_lat_force.Add(snap.calc_rear_lat_force);

        // --- Header C: Raw Telemetry ---
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

        // Updates for extra buffers
        plot_raw_slip_angle.Add(snap.raw_front_slip_angle);
        plot_raw_rear_slip_angle.Add(snap.raw_rear_slip_angle);
        plot_raw_front_deflection.Add(snap.raw_front_deflection);

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

    // --- Header A: FFB Components (Output) ---
    // [Main Forces], [Modifiers], [Textures]
    if (ImGui::CollapsingHeader("A. FFB Components (Output)", ImGuiTreeNodeFlags_DefaultOpen)) {
        PlotWithStats("Total Output", plot_total, -1.0f, 1.0f, ImVec2(0, 60), 
                      "Final FFB Output (-1.0 to 1.0)");
        
        ImGui::Separator();
        ImGui::Columns(3, "FFBMain", false);
        
        // Group: Main Forces
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "[Main Forces]");
        
        PlotWithStats("Base Torque (Nm)", plot_base, -30.0f, 30.0f, ImVec2(0, 40),
                      "Steering Rack Force derived from Game Physics");
        
        PlotWithStats("SoP (Base Chassis G)", plot_sop, -20.0f, 20.0f, ImVec2(0, 40),
                      "Force from Lateral G-Force (Seat of Pants)");

        PlotWithStats("Yaw Kick", plot_yaw_kick, -20.0f, 20.0f, ImVec2(0, 40),
                      "Force from Yaw Acceleration (Rotation Kick)");
        
        PlotWithStats("Rear Align Torque", plot_rear_torque, -20.0f, 20.0f, ImVec2(0, 40),
                      "Force from Rear Lateral Force");
        
        PlotWithStats("Gyro Damping", plot_gyro_damping, -20.0f, 20.0f, ImVec2(0, 40),
                      "Synthetic damping force");

        PlotWithStats("Scrub Drag Force", plot_scrub_drag, -20.0f, 20.0f, ImVec2(0, 40),
                      "Resistance force from sideways tire dragging");
        
        ImGui::NextColumn();
        
        // Group: Modifiers
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.7f, 1.0f), "[Modifiers]");
        
        PlotWithStats("Oversteer Boost", plot_oversteer, -20.0f, 20.0f, ImVec2(0, 40),
                      "Added force from Rear Grip loss");
        
        PlotWithStats("Understeer Cut", plot_understeer, -20.0f, 20.0f, ImVec2(0, 40),
                      "Reduction in force due to front grip loss");
        
        PlotWithStats("Clipping", plot_clipping, 0.0f, 1.1f, ImVec2(0, 40),
                      "Indicates when Output hits max limit");
        
        ImGui::NextColumn();
        
        // Group: Textures
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "[Textures]");
        
        PlotWithStats("Road Texture", plot_road, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Suspension Velocity");
        PlotWithStats("Slide Texture", plot_slide, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Lateral Scrubbing");
        PlotWithStats("Lockup Vib", plot_lockup, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Wheel Lockup");
        PlotWithStats("Spin Vib", plot_spin, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Wheel Spin");
        PlotWithStats("Bottoming", plot_bottoming, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Suspension Bottoming");

        ImGui::Columns(1);
    }

    // --- Header B: Internal Physics (Brain) ---
    // [Loads], [Grip/Slip], [Forces]
    if (ImGui::CollapsingHeader("B. Internal Physics (Brain)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(3, "PhysCols", false);
        
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
        
        ImGui::NextColumn();
        
        // Group: Grip/Slip
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Grip/Slip]");
        
        PlotWithStats("Calc Front Grip", plot_calc_front_grip, 0.0f, 1.2f, ImVec2(0, 40),
                      "Grip used for physics math (approximated if missing)");
        
        PlotWithStats("Calc Rear Grip", plot_calc_rear_grip, 0.0f, 1.2f, ImVec2(0, 40),
                      "Rear Grip used for SoP/Oversteer math");
        
        PlotWithStats("Front Slip Ratio", plot_calc_slip_ratio, -1.0f, 1.0f, ImVec2(0, 40),
                      "Calculated or Game-provided Slip Ratio");
        
        PlotWithStats("Front Slip Angle (Sm)", plot_calc_slip_angle_smoothed, 0.0f, 1.0f, ImVec2(0, 40),
                      "Smoothed Slip Angle (LPF) used for approximation");
        
        PlotWithStats("Rear Slip Angle (Sm)", plot_calc_rear_slip_angle_smoothed, 0.0f, 1.0f, ImVec2(0, 40),
                      "Smoothed Rear Slip Angle (LPF)");
        
        ImGui::NextColumn();
        
        // Group: Forces
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Forces]");
        
        PlotWithStats("Calc Rear Lat Force", plot_calc_rear_lat_force, -5000.0f, 5000.0f, ImVec2(0, 40),
                      "Calculated Rear Lateral Force (Workaround)");

        ImGui::Columns(1);
    }

    // --- Header C: Raw Game Telemetry (Input) ---
    // [Driver Input], [Vehicle State], [Raw Tire Data], [Patch Velocities]
    if (ImGui::CollapsingHeader("C. Raw Game Telemetry (Input)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(4, "TelCols", false);
        
        // Group: Driver Input
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Driver Input]");
        
        PlotWithStats("Steering Torque", plot_raw_steer, -30.0f, 30.0f, ImVec2(0, 40),
                      "Raw Steering Torque from Game API");
        
        PlotWithStats("Steering Input", plot_raw_input_steering, -1.0f, 1.0f, ImVec2(0, 40),
                      "Driver wheel position -1 to 1");
        
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
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red for Brake
        ImGui::PlotLines("##BrkComb", plot_raw_brake.data.data(), (int)plot_raw_brake.data.size(), plot_raw_brake.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Green: Throttle, Red: Brake");
        ImGui::SetCursorScreenPos(pos); // Reset
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green for Throttle
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent Bg
        ImGui::PlotLines("##ThrComb", plot_raw_throttle.data.data(), (int)plot_raw_throttle.data.size(), plot_raw_throttle.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
        ImGui::PopStyleColor(2);
        
        ImGui::NextColumn();
        
        // Group: Vehicle State
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Vehicle State]");
        
        PlotWithStats("Chassis Lat Accel", plot_input_accel, -20.0f, 20.0f, ImVec2(0, 40),
                      "Local Lateral Acceleration (G)");
        
        PlotWithStats("Car Speed (m/s)", plot_raw_car_speed, 0.0f, 100.0f, ImVec2(0, 40),
                      "Vehicle Speed");
        
        ImGui::NextColumn();
        
        // Group: Raw Tire Data
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Raw Tire Data]");
        
        // Raw Front Load with warning label and stats
        {
            float current = plot_raw_load.GetCurrent();
            float min_val = plot_raw_load.GetMin();
            float max_val = plot_raw_load.GetMax();
            char stats_label[256];
            snprintf(stats_label, sizeof(stats_label), "Raw Front Load | Val: %.4f | Min: %.3f | Max: %.3f", 
                     current, min_val, max_val);
            
            if (g_warn_load) ImGui::TextColored(ImVec4(1,0,0,1), "%s (MISSING)", stats_label);
            else ImGui::Text("%s", stats_label);
            
            ImGui::PlotLines("##RawLoad", plot_raw_load.data.data(), (int)plot_raw_load.data.size(), 
                           plot_raw_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raw Tire Load from Game API");
        }
        
        // Raw Front Grip with warning label and stats
        {
            float current = plot_raw_grip.GetCurrent();
            float min_val = plot_raw_grip.GetMin();
            float max_val = plot_raw_grip.GetMax();
            char stats_label[256];
            snprintf(stats_label, sizeof(stats_label), "Raw Front Grip | Val: %.4f | Min: %.3f | Max: %.3f", 
                     current, min_val, max_val);
            
            if (g_warn_grip) ImGui::TextColored(ImVec4(1,0,0,1), "%s (MISSING)", stats_label);
            else ImGui::Text("%s", stats_label);
            
            ImGui::PlotLines("##RawGrip", plot_raw_grip.data.data(), (int)plot_raw_grip.data.size(), 
                           plot_raw_grip.offset, NULL, 0.0f, 1.2f, ImVec2(0, 40));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raw Grip Fraction from Game API");
        }
        
        PlotWithStats("Raw Rear Grip", plot_raw_rear_grip, 0.0f, 1.2f, ImVec2(0, 40),
                      "Raw Rear Grip Fraction from Game API");

        ImGui::NextColumn();
        
        // Group: Patch Velocities
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Patch Velocities]");
        
        PlotWithStats("Avg Front Lat PatchVel", plot_raw_front_lat_patch_vel, 0.0f, 20.0f, ImVec2(0, 40),
                      "Lateral Velocity at Contact Patch");
        
        PlotWithStats("Avg Rear Lat PatchVel", plot_raw_rear_lat_patch_vel, 0.0f, 20.0f, ImVec2(0, 40),
                      "Lateral Velocity at Contact Patch (Rear)");

        PlotWithStats("Avg Front Long PatchVel", plot_raw_front_long_patch_vel, -20.0f, 20.0f, ImVec2(0, 40),
                      "Longitudinal Velocity at Contact Patch (Front)");

        PlotWithStats("Avg Rear Long PatchVel", plot_raw_rear_long_patch_vel, -20.0f, 20.0f, ImVec2(0, 40),
                      "Longitudinal Velocity at Contact Patch (Rear)");

        ImGui::Columns(1);
    }

    ImGui::End();
}
