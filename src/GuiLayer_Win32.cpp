#include "GuiLayer.h"
#include "GuiPlatform.h"
#include "Version.h"
#include "Logger.h"
#include "Config.h"
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <chrono>

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

static const int MIN_WINDOW_WIDTH = 400;
static const int MIN_WINDOW_HEIGHT = 600;

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern std::atomic<bool> g_running;

class Win32GuiPlatform : public IGuiPlatform {
public:
    void SetAlwaysOnTop(bool enabled) override {
        if (!g_hwnd) return;
        HWND insertAfter = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
        ::SetWindowPos(g_hwnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void ResizeWindow(int x, int y, int w, int h) override {
        if (w < MIN_WINDOW_WIDTH) w = MIN_WINDOW_WIDTH;
        if (h < MIN_WINDOW_HEIGHT) h = MIN_WINDOW_HEIGHT;
        ::SetWindowPos(g_hwnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void SaveWindowGeometry(bool is_graph_mode) override {
        RECT rect;
        if (::GetWindowRect(g_hwnd, &rect)) {
            Config::win_pos_x = rect.left;
            Config::win_pos_y = rect.top;
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            if (w < MIN_WINDOW_WIDTH) w = MIN_WINDOW_WIDTH;
            if (h < MIN_WINDOW_HEIGHT) h = MIN_WINDOW_HEIGHT;
            if (is_graph_mode) {
                Config::win_w_large = w;
                Config::win_h_large = h;
            } else {
                Config::win_w_small = w;
                Config::win_h_small = h;
            }
        }
    }

    bool OpenPresetFileDialog(std::string& outPath) override {
        char filename[MAX_PATH] = "";
        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = g_hwnd;
        ofn.lpstrFilter = "Preset Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "ini";
        if (GetOpenFileNameA(&ofn)) {
            outPath = filename;
            return true;
        }
        return false;
    }

    bool SavePresetFileDialog(std::string& outPath, const std::string& defaultName) override {
        char filename[MAX_PATH] = "";
        strncpy_s(filename, defaultName.c_str(), _TRUNCATE);
        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = g_hwnd;
        ofn.lpstrFilter = "Preset Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "ini";
        if (GetSaveFileNameA(&ofn)) {
            outPath = filename;
            return true;
        }
        return false;
    }

    void* GetWindowHandle() override {
        return (void*)g_hwnd;
    }
};

static Win32GuiPlatform g_platform;
IGuiPlatform& GetGuiPlatform() { return g_platform; }

// Compatibility Helpers
void ResizeWindowPlatform(int x, int y, int w, int h) { GetGuiPlatform().ResizeWindow(x, y, w, h); }
void SaveCurrentWindowGeometryPlatform(bool is_graph_mode) { GetGuiPlatform().SaveWindowGeometry(is_graph_mode); }
void SetWindowAlwaysOnTopPlatform(bool enabled) { GetGuiPlatform().SetAlwaysOnTop(enabled); }
bool OpenPresetFileDialogPlatform(std::string& outPath) { return GetGuiPlatform().OpenPresetFileDialog(outPath); }
bool SavePresetFileDialogPlatform(std::string& outPath, const std::string& defaultName) { return GetGuiPlatform().SavePresetFileDialog(outPath, defaultName); }


bool GuiLayer::Init() {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"lmuFFB", NULL };
    ::RegisterClassExW(&wc);
    std::string ver = LMUFFB_VERSION;
    std::wstring wver(ver.begin(), ver.end());
    std::wstring title = L"lmuFFB v" + wver;
    int start_w = Config::show_graphs ? Config::win_w_large : Config::win_w_small;
    int start_h = Config::show_graphs ? Config::win_h_large : Config::win_h_small;
    if (start_w < MIN_WINDOW_WIDTH) start_w = MIN_WINDOW_WIDTH;
    if (start_h < MIN_WINDOW_HEIGHT) start_h = MIN_WINDOW_HEIGHT;
    int pos_x = Config::win_pos_x, pos_y = Config::win_pos_y;
    RECT workArea; SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    if (pos_x < workArea.left - 100 || pos_x > workArea.right - 100 || pos_y < workArea.top - 100 || pos_y > workArea.bottom - 100) {
        pos_x = 100; pos_y = 100;
    }
    g_hwnd = ::CreateWindowW(wc.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, pos_x, pos_y, start_w, start_h, NULL, NULL, wc.hInstance, NULL);
    if (!g_hwnd) {
        Logger::Get().LogWin32Error("CreateWindowW", GetLastError());
        return false;
    }
    Logger::Get().Log("Window Created: %p", g_hwnd);

    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D(); ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        Logger::Get().Log("Failed to create D3D Device.");
        return false;
    }
    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT); ::UpdateWindow(g_hwnd);
    if (Config::m_always_on_top) SetWindowAlwaysOnTopPlatform(true);
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    SetupGUIStyle();
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    return true;
}

void GuiLayer::Shutdown(FFBEngine& engine) {
    SaveCurrentWindowGeometryPlatform(Config::show_graphs);
    Config::Save(engine);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(L"lmuFFB", GetModuleHandle(NULL));
}

void* GuiLayer::GetWindowHandle() {
    return (void*)g_hwnd;
}

bool GuiLayer::Render(FFBEngine& engine) {
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg); ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) { g_running = false; return false; }
    }
    if (g_running == false) return false;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    DrawTuningWindow(engine);
    if (Config::show_graphs) DrawDebugWindow(engine);
    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) || ImGui::IsAnyItemActive();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            Logger::Get().Log("ResizeBuffers: %d x %d", LOWORD(lParam), HIWORD(lParam));
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY: ::PostQuitMessage(0); return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2; sd.BufferDesc.Width = 0; sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL featureLevel; const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (hr != S_OK) {
        Logger::Get().LogWin32Error("D3D11CreateDeviceAndSwapChain", hr);
        return false;
    }
    Logger::Get().Log("D3D11 Device Created. Feature Level: 0x%X", featureLevel);
    CreateRenderTarget(); return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView); pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

#else
// Stub Implementation for Headless Builds
bool GuiLayer::Init() {
    return true;
}
void GuiLayer::Shutdown(FFBEngine& engine) {
    Config::Save(engine);
}
bool GuiLayer::Render(FFBEngine& engine) { return false; }
void* GuiLayer::GetWindowHandle() { return nullptr; }

void ResizeWindowPlatform(int x, int y, int w, int h) {}
void SaveCurrentWindowGeometryPlatform(bool is_graph_mode) {}
void SetWindowAlwaysOnTopPlatform(bool enabled) {}
bool OpenPresetFileDialogPlatform(std::string& outPath) { return false; }
bool SavePresetFileDialogPlatform(std::string& outPath, const std::string& defaultName) { return false; }

#endif
