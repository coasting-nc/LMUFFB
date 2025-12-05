#include "GuiLayer.h"
#include <iostream>

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

bool GuiLayer::Init() {
    // Create Application Window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"LMUFFB", NULL };
    ::RegisterClassExW(&wc);
    g_hwnd = ::CreateWindowW(wc.lpszClassName, L"LMUFFB Config", WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

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
    ImGui::Begin("FFB Configuration");

    ImGui::Text("Core Settings");
    ImGui::SliderFloat("Master Gain", &engine.m_gain, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Smoothing", &engine.m_smoothing, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f");

    ImGui::Separator();
    ImGui::Text("Effects");
    ImGui::SliderFloat("Understeer (Grip)", &engine.m_understeer_effect, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("SoP (Lateral G)", &engine.m_sop_effect, 0.0f, 2.0f, "%.2f");

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
    // Visualize Clipping (this requires the calculated force from the engine passed back, 
    // or we just show the static gain for now. A real app needs a shared state for 'last_output_force')
    // ImGui::ProgressBar((float)engine.last_force, ImVec2(0.0f, 0.0f)); 
    ImGui::Text("Clipping Visualization Placeholder");

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
