#include "GuiLayer.h"
#include "Version.h"
#include "Config.h"
#include "DirectInputFFB.h"
#include "GameConnector.h"
#include "GuiWidgets.h"
#include "AsyncLogger.h"
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <chrono>

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
//#include "implot.h"
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

// v0.5.5 Layout Constants
static const float CONFIG_PANEL_WIDTH = 500.0f;  // Width of config panel when graphs are visible
static const int MIN_WINDOW_WIDTH = 400;         // Minimum window width to keep UI usable
static const int MIN_WINDOW_HEIGHT = 600;        // Minimum window height to keep UI usable

// v0.5.7 Latency Warning Threshold
static const int LATENCY_WARNING_THRESHOLD_MS = 15; // Green if < 15ms, Red if >= 15ms

// v0.6.5 PrintWindow flag (define if not available in SDK)
#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

static constexpr std::chrono::seconds CONNECT_ATTEMPT_INTERVAL(2);

  // Forward declarations of helper functions
bool OpenPresetFileDialog(HWND hwnd, std::string& outPath);
bool SavePresetFileDialog(HWND hwnd, std::string& outPath, const std::string& defaultName);

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void SetWindowAlwaysOnTop(HWND hwnd, bool enabled); 

// v0.5.5 Window Management Helpers

/**
 * Resizes the OS window with minimum size enforcement.
 * Ensures window dimensions never fall below usability thresholds.
 */
void ResizeWindow(HWND hwnd, int x, int y, int w, int h) {
    // Enforce minimum dimensions to prevent UI from becoming unusable
    if (w < MIN_WINDOW_WIDTH) w = MIN_WINDOW_WIDTH;
    if (h < MIN_WINDOW_HEIGHT) h = MIN_WINDOW_HEIGHT;
    
    ::SetWindowPos(hwnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
 * Saves current window geometry to Config static variables.
 * Stores position and dimensions based on current mode (small vs large).
 * 
 * @param is_graph_mode If true, saves to win_w_large/win_h_large; otherwise to win_w_small/win_h_small
 */
void SaveCurrentWindowGeometry(bool is_graph_mode) {
    RECT rect;
    if (::GetWindowRect(g_hwnd, &rect)) {
        Config::win_pos_x = rect.left;
        Config::win_pos_y = rect.top;
        int w = rect.right - rect.left;
        int h = rect.bottom - rect.top;

        // Enforce minimum dimensions before saving
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

// External linkage to FFB loop status
extern std::atomic<bool> g_running;
extern std::mutex g_engine_mutex;

// Macro stringification helper
#define XSTR(x) STR(x)
#define STR(x) #x

// VERSION is now defined in Version.h

// NEW: Professional "Flat Dark" Theme
void GuiLayer::SetupGUIStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // 1. Geometry
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    
    // 2. Colors
    ImVec4* colors = style.Colors;
    
    // Backgrounds: Deep Grey
    colors[ImGuiCol_WindowBg]       = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]        = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg]        = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);
    
    // Headers: Transparent (Just text highlight)
    colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.20f, 0.20f, 0.00f); // Transparent!
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    
    // Controls (Sliders/Buttons): Dark Grey container
    colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    
    // Accents (The Data): Bright Blue/Teal
    // This draws the eye ONLY to the values
    ImVec4 accent = ImVec4(0.00f, 0.60f, 0.85f, 1.00f); 
    colors[ImGuiCol_SliderGrab]     = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]  = accent;
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.00f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_CheckMark]      = accent;
    
    // Text
    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}

bool GuiLayer::Init() {
    // Create Application Window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"lmuFFB", NULL };
    ::RegisterClassExW(&wc);
    
    // Construct Title with Version
    // We need wide string for CreateWindowW. 
    // Simplified conversion for version string (assumes ASCII version)
    std::string ver = LMUFFB_VERSION;
    std::wstring wver(ver.begin(), ver.end());
    std::wstring title = L"lmuFFB v" + wver;

    // 1. Determine startup size with validation
    int start_w = Config::show_graphs ? Config::win_w_large : Config::win_w_small;
    int start_h = Config::show_graphs ? Config::win_h_large : Config::win_h_small;
    
    // Enforce minimum dimensions
    if (start_w < MIN_WINDOW_WIDTH) start_w = MIN_WINDOW_WIDTH;
    if (start_h < MIN_WINDOW_HEIGHT) start_h = MIN_WINDOW_HEIGHT;
    
    // 2. Validate window position (ensure it's on-screen)
    int pos_x = Config::win_pos_x;
    int pos_y = Config::win_pos_y;
    
    // Get primary monitor work area
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    
    // If saved position would place window completely off-screen, reset to default
    if (pos_x < workArea.left - 100 || pos_x > workArea.right - 100 ||
        pos_y < workArea.top - 100 || pos_y > workArea.bottom - 100) {
        pos_x = 100;  // Reset to safe default
        pos_y = 100;
        Config::win_pos_x = pos_x;  // Update config
        Config::win_pos_y = pos_y;
    }

    // 3. Create Window with validated position and size
    g_hwnd = ::CreateWindowW(wc.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, 
        pos_x, pos_y, 
        start_w, start_h, 
        NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    // Show the window
    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);

    // Apply saved "Always on Top" setting now that window is shown
    if (Config::m_always_on_top) {
        SetWindowAlwaysOnTop(g_hwnd, true);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    SetupGUIStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    return true;
}

void GuiLayer::Shutdown(FFBEngine& engine) {
    // Capture the final position/size before destroying the window
    SaveCurrentWindowGeometry(Config::show_graphs);

    // Call Save to persist all settings (Auto-save on shutdown v0.6.25)
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
    if (Config::show_graphs) {
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

// Helper function to capture a window using PrintWindow API (works for console and all window types)
bool CaptureWindowToBuffer(HWND hwnd, std::vector<unsigned char>& buffer, int& width, int& height) {
    if (!hwnd || !IsWindow(hwnd)) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: Invalid window handle" << std::endl;
        return false;
    }
    
    // Get window dimensions
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: GetWindowRect failed, error: " << GetLastError() << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] GetWindowRect returned: left=" << rect.left << ", top=" << rect.top 
              << ", right=" << rect.right << ", bottom=" << rect.bottom << std::endl;
    
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    
    // Special case: Console windows sometimes return (0,0,0,0) from GetWindowRect
    // even though they have a valid handle. Try GetClientRect as fallback.
    if (width <= 0 || height <= 0) {
        std::cout << "[DEBUG] GetWindowRect returned invalid dimensions, trying GetClientRect..." << std::endl;
        
        RECT clientRect;
        if (GetClientRect(hwnd, &clientRect)) {
            width = clientRect.right - clientRect.left;
            height = clientRect.bottom - clientRect.top;
            
            std::cout << "[DEBUG] GetClientRect returned: " << width << "x" << height << std::endl;
            
            // If we got valid dimensions from GetClientRect, we need to convert to screen coordinates
            if (width > 0 && height > 0) {
                POINT topLeft = {0, 0};
                if (ClientToScreen(hwnd, &topLeft)) {
                    rect.left = topLeft.x;
                    rect.top = topLeft.y;
                    rect.right = topLeft.x + width;
                    rect.bottom = topLeft.y + height;
                    std::cout << "[DEBUG] Converted to screen coordinates: (" << rect.left << "," << rect.top 
                              << ") to (" << rect.right << "," << rect.bottom << ")" << std::endl;
                }
            }
        }
    }
    
    std::cout << "[DEBUG] Final calculated dimensions: " << width << "x" << height << std::endl;
    
    if (width <= 0 || height <= 0) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: Invalid dimensions " << width << "x" << height << std::endl;
        std::cout << "[DEBUG] This usually means the window is minimized, hidden, or not properly initialized" << std::endl;
        return false;
    }
    
    // Get screen DC for creating compatible bitmap
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: GetDC(NULL) failed" << std::endl;
        return false;
    }
    
    // Create compatible DC and bitmap
    HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
    if (!hdcMemDC) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: CreateCompatibleDC failed" << std::endl;
        ReleaseDC(NULL, hdcScreen);
        return false;
    }
    
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, width, height);
    if (!hbmScreen) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: CreateCompatibleBitmap failed" << std::endl;
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return false;
    }
    
    // Select bitmap into memory DC
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMemDC, hbmScreen);
    
    // Try PrintWindow first (works for most windows)
    bool captureSuccess = false;
    std::string captureMethod = "";
    
    if (PrintWindow(hwnd, hdcMemDC, PW_RENDERFULLCONTENT)) {
        captureSuccess = true;
        captureMethod = "PrintWindow with PW_RENDERFULLCONTENT";
    } else if (PrintWindow(hwnd, hdcMemDC, 0)) {
        captureSuccess = true;
        captureMethod = "PrintWindow without flags";
    } else {
        // Fallback: Use BitBlt to capture from screen coordinates
        // This works for console windows and other special windows
        std::cout << "[DEBUG] PrintWindow failed, trying BitBlt fallback..." << std::endl;
        if (BitBlt(hdcMemDC, 0, 0, width, height, hdcScreen, rect.left, rect.top, SRCCOPY)) {
            captureSuccess = true;
            captureMethod = "BitBlt from screen coordinates";
        } else {
            std::cout << "[DEBUG] BitBlt also failed! Error code: " << GetLastError() << std::endl;
        }
    }
    
    if (!captureSuccess) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: All capture methods failed" << std::endl;
        SelectObject(hdcMemDC, hbmOld);
        DeleteObject(hbmScreen);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return false;
    }
    
    std::cout << "[DEBUG] Capture successful using: " << captureMethod << std::endl;
    
    // Get bitmap data
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Negative for top-down bitmap
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    
    // Allocate buffer
    buffer.resize(width * height * 4);
    
    // Get bits from bitmap
    if (!GetDIBits(hdcMemDC, hbmScreen, 0, height, buffer.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
        std::cout << "[DEBUG] CaptureWindowToBuffer failed: GetDIBits failed" << std::endl;
        SelectObject(hdcMemDC, hbmOld);
        DeleteObject(hbmScreen);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return false;
    }
    
    // Convert BGRA to RGBA
    for (int i = 0; i < width * height; ++i) {
        int idx = i * 4;
        unsigned char b = buffer[idx + 0];
        unsigned char r = buffer[idx + 2];
        buffer[idx + 0] = r;
        buffer[idx + 2] = b;
        buffer[idx + 3] = 255; // Force opaque
    }
    
    // Cleanup
    SelectObject(hdcMemDC, hbmOld);
    DeleteObject(hbmScreen);
    DeleteDC(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    
    return true;
}

// Composite Screenshot: Captures both GUI and Console windows side-by-side
void SaveCompositeScreenshot(const char* filename) {
    HWND guiWindow = g_hwnd;
    HWND consoleWindow = GetConsoleWindow();
    
    std::vector<unsigned char> guiBuffer, consoleBuffer;
    int guiWidth = 0, guiHeight = 0;
    int consoleWidth = 0, consoleHeight = 0;
    
    // Capture GUI window
    bool hasGui = CaptureWindowToBuffer(guiWindow, guiBuffer, guiWidth, guiHeight);
    std::cout << "[GUI] GUI window capture: " << (hasGui ? "SUCCESS" : "FAILED") << std::endl;
    
    // Capture Console window (if exists)
    bool hasConsole = false;
    if (consoleWindow) {
        // Check if console window is actually visible
        bool isVisible = IsWindowVisible(consoleWindow);
        std::cout << "[GUI] Console window found (HWND: " << consoleWindow << "), visible: " << (isVisible ? "YES" : "NO") << std::endl;
        
        if (isVisible) {
            std::cout << "[GUI] Attempting to capture visible console window..." << std::endl;
            
            // Try to get console screen buffer info to determine actual size
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            
            if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
                // Calculate console window size from buffer info
                int consoleWidthChars = csbi.srWindow.Right - csbi.srWindow.Left + 1;
                int consoleHeightChars = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
                
                std::cout << "[DEBUG] Console buffer info: " << consoleWidthChars << " cols x " 
                          << consoleHeightChars << " rows" << std::endl;
                
                // Get console font size to calculate pixel dimensions
                CONSOLE_FONT_INFO cfi;
                int fontWidth = 8;   // Default Consolas/Courier font width
                int fontHeight = 16; // Default font height
                
                if (GetCurrentConsoleFont(hConsole, FALSE, &cfi)) {
                    if (cfi.dwFontSize.X > 0) fontWidth = cfi.dwFontSize.X;
                    if (cfi.dwFontSize.Y > 0) fontHeight = cfi.dwFontSize.Y;
                }
                
                // Estimate console window size in pixels
                // Add some padding for window borders/title bar
                int estimatedWidth = consoleWidthChars * fontWidth + 20;  // 20px for borders
                int estimatedHeight = consoleHeightChars * fontHeight + 60; // 60px for title bar + borders
                
                std::cout << "[DEBUG] Estimated console size: " << estimatedWidth << "x" << estimatedHeight 
                          << " (font: " << fontWidth << "x" << fontHeight << ")" << std::endl;
                
                // Try to find console window by enumerating all top-level windows
                // and looking for one with similar dimensions
                struct FindConsoleData {
                    HWND consoleHwnd;
                    HWND foundHwnd;
                    int targetWidth;
                    int targetHeight;
                } findData = { consoleWindow, NULL, estimatedWidth, estimatedHeight };
                
                EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                    FindConsoleData* data = (FindConsoleData*)lParam;
                    
                    if (!IsWindowVisible(hwnd)) return TRUE;
                    
                    RECT rect;
                    if (GetWindowRect(hwnd, &rect)) {
                        int w = rect.right - rect.left;
                        int h = rect.bottom - rect.top;
                        
                        // Look for window with dimensions close to our estimate (within 30%)
                        if (w > 0 && h > 0) {
                            int widthDiff = abs(w - data->targetWidth);
                            int heightDiff = abs(h - data->targetHeight);
                            
                            if (widthDiff < data->targetWidth * 0.3 && heightDiff < data->targetHeight * 0.3) {
                                char title[512];
                                GetWindowTextA(hwnd, title, sizeof(title));
                                
                                std::cout << "[DEBUG] Found window with similar size: \"" << title 
                                          << "\" (" << w << "x" << h << ", HWND: " << hwnd << ")" << std::endl;
                                
                                // Check if title contains our exe name or path
                                std::string titleStr(title);
                                if (titleStr.find("LMUFFB") != std::string::npos ||
                                    titleStr.find("lmuFFB") != std::string::npos ||
                                    titleStr.find(".exe") != std::string::npos ||
                                    titleStr.find("development") != std::string::npos) {
                                    data->foundHwnd = hwnd;
                                    return FALSE; // Stop enumeration
                                }
                            }
                        }
                    }
                    return TRUE;
                }, (LPARAM)&findData);
                
                if (findData.foundHwnd) {
                    std::cout << "[GUI] Found console window by size matching, attempting capture..." << std::endl;
                    hasConsole = CaptureWindowToBuffer(findData.foundHwnd, consoleBuffer, consoleWidth, consoleHeight);
                }
            }
            
            std::cout << "[GUI] Console window capture: " << (hasConsole ? "SUCCESS" : "FAILED") << std::endl;
            if (hasConsole) {
                std::cout << "[GUI] Console dimensions: " << consoleWidth << "x" << consoleHeight << std::endl;
            }
        } else {
            std::cout << "[GUI] Console window is not visible, skipping capture" << std::endl;
        }
    } else {
        std::cout << "[GUI] No console window found (GetConsoleWindow returned NULL)" << std::endl;
    }
    
    if (!hasGui && !hasConsole) {
        std::cout << "[GUI] Screenshot failed: No windows to capture" << std::endl;
        return;
    }
    
    // If only one window exists, save it directly
    if (!hasConsole) {
        stbi_write_png(filename, guiWidth, guiHeight, 4, guiBuffer.data(), guiWidth * 4);
        std::cout << "[GUI] Screenshot saved (GUI only) to " << filename << std::endl;
        return;
    }
    
    if (!hasGui) {
        stbi_write_png(filename, consoleWidth, consoleHeight, 4, consoleBuffer.data(), consoleWidth * 4);
        std::cout << "[GUI] Screenshot saved (Console only) to " << filename << std::endl;
        return;
    }
    
    // Composite both windows side-by-side
    // Layout: [GUI] [10px gap] [Console]
    const int gap = 10;
    int compositeWidth = guiWidth + gap + consoleWidth;
    int compositeHeight = (std::max)(guiHeight, consoleHeight);
    
    std::vector<unsigned char> compositeBuffer(compositeWidth * compositeHeight * 4, 0);
    
    // Fill background with dark gray
    for (int i = 0; i < compositeWidth * compositeHeight; ++i) {
        int idx = i * 4;
        compositeBuffer[idx + 0] = 30;  // R
        compositeBuffer[idx + 1] = 30;  // G
        compositeBuffer[idx + 2] = 30;  // B
        compositeBuffer[idx + 3] = 255; // A
    }
    
    // Copy GUI window to left side
    for (int y = 0; y < guiHeight; ++y) {
        for (int x = 0; x < guiWidth; ++x) {
            int srcIdx = (y * guiWidth + x) * 4;
            int dstIdx = (y * compositeWidth + x) * 4;
            compositeBuffer[dstIdx + 0] = guiBuffer[srcIdx + 0];
            compositeBuffer[dstIdx + 1] = guiBuffer[srcIdx + 1];
            compositeBuffer[dstIdx + 2] = guiBuffer[srcIdx + 2];
            compositeBuffer[dstIdx + 3] = guiBuffer[srcIdx + 3];
        }
    }
    
    // Copy Console window to right side (after gap)
    int consoleOffsetX = guiWidth + gap;
    for (int y = 0; y < consoleHeight; ++y) {
        for (int x = 0; x < consoleWidth; ++x) {
            int srcIdx = (y * consoleWidth + x) * 4;
            int dstIdx = (y * compositeWidth + (consoleOffsetX + x)) * 4;
            compositeBuffer[dstIdx + 0] = consoleBuffer[srcIdx + 0];
            compositeBuffer[dstIdx + 1] = consoleBuffer[srcIdx + 1];
            compositeBuffer[dstIdx + 2] = consoleBuffer[srcIdx + 2];
            compositeBuffer[dstIdx + 3] = consoleBuffer[srcIdx + 3];
        }
    }
    
    // Save composite image
    stbi_write_png(filename, compositeWidth, compositeHeight, 4, compositeBuffer.data(), compositeWidth * 4);
    
    std::cout << "[GUI] Composite screenshot saved to " << filename 
              << " (GUI: " << guiWidth << "x" << guiHeight 
              << ", Console: " << consoleWidth << "x" << consoleHeight << ")" << std::endl;
}

// Screenshot Helper (DirectX 11) - Legacy single-window capture
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

    // --- A. LAYOUT CALCULATION (v0.5.5 Smart Container) ---
    ImGuiViewport* viewport = ImGui::GetMainViewport(); 

    // Calculate width: Full viewport if graphs off, fixed width if graphs on
    float current_width = Config::show_graphs ? CONFIG_PANEL_WIDTH : viewport->Size.x;

    // Lock the ImGui window to the left side of the OS window
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(current_width, viewport->Size.y));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("MainUI", nullptr, flags);

    // Header Text
    ImGui::TextColored(ImVec4(1, 1, 1, 0.4f), "lmuFFB v%s", LMUFFB_VERSION);
    ImGui::Separator();

    // Connection Status
    static std::chrono::steady_clock::time_point last_check_time =
        std::chrono::steady_clock::now();
    
    if (!GameConnector::Get().IsConnected()) {
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Connecting to LMU...");
      if (std::chrono::steady_clock::now() - last_check_time >
          CONNECT_ATTEMPT_INTERVAL) {
        last_check_time = std::chrono::steady_clock::now();
        GameConnector::Get().TryConnect();
      }
    } else {
      ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected to LMU");
    }

    // --- 1. TOP BAR (System Status & Quick Controls) ---
    // Keep this outside columns for full width awareness
    
    // Device Selection
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
        for (int i = 0; i < devices.size(); i++) {
            bool is_selected = (selected_device_idx == i);
            if (ImGui::Selectable(devices[i].name.c_str(), is_selected)) {
                selected_device_idx = i;
                DirectInputFFB::Get().SelectDevice(devices[i].guid);
                Config::m_last_device_guid = DirectInputFFB::GuidToString(devices[i].guid);
                Config::Save(engine); 
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Rescan")) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        selected_device_idx = -1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Unbind")) {
        DirectInputFFB::Get().ReleaseDevice();
        selected_device_idx = -1;
    }

    // Acquisition Mode & Troubleshooting
    if (DirectInputFFB::Get().IsActive()) {
        if (DirectInputFFB::Get().IsExclusive()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Mode: EXCLUSIVE (Game FFB Blocked)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("lmuFFB has exclusive control.\nThe game can read steering but cannot send FFB.\nThis prevents 'Double FFB' issues.");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Mode: SHARED (Potential Conflict)");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("lmuFFB is sharing the device.\nEnsure In-Game FFB is disabled\nto avoid LMU reacquiring the device.");
        }
    }

    if (ImGui::Checkbox("Always on Top", &Config::m_always_on_top)) {
        SetWindowAlwaysOnTop(g_hwnd, Config::m_always_on_top);
        Config::Save(engine);
    }
    ImGui::SameLine();
    
    // --- B. THE CHECKBOX LOGIC (v0.5.5 Reactive Resize) ---
    bool toggled = Config::show_graphs;
    if (ImGui::Checkbox("Graphs", &toggled)) {
        // 1. Save the geometry of the OLD state before switching
        SaveCurrentWindowGeometry(Config::show_graphs);
        
        // 2. Update state
        Config::show_graphs = toggled;
        
        // 3. Apply geometry of the NEW state
        int target_w = Config::show_graphs ? Config::win_w_large : Config::win_w_small;
        int target_h = Config::show_graphs ? Config::win_h_large : Config::win_h_small;
        
        // Resize the OS window immediately
        ResizeWindow(g_hwnd, Config::win_pos_x, Config::win_pos_y, target_w, target_h);
        
        // Force immediate save of state
        Config::Save(engine);
    }
    
    // --- Telemetry Logger ---
    ImGui::Separator();
    bool is_logging = AsyncLogger::Get().IsLogging();
    if (is_logging) {
         if (ImGui::Button("STOP LOG", ImVec2(80, 0))) {
             AsyncLogger::Get().Stop();
         }
         ImGui::SameLine();
         // Pulse effect or color
         float time = (float)ImGui::GetTime();
         bool blink = (fmod(time, 1.0f) < 0.5f);
         ImGui::TextColored(blink ? ImVec4(1,0,0,1) : ImVec4(0.6f,0,0,1), "REC");
         
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
    } else {
         if (ImGui::Button("START LOGGING", ImVec2(120, 0))) {
             SessionInfo info;
             info.app_version = LMUFFB_VERSION;
             if (engine.m_vehicle_name[0] != '\0') info.vehicle_name = engine.m_vehicle_name;
             else info.vehicle_name = "UnknownCar";
             
             if (engine.m_track_name[0] != '\0') info.track_name = engine.m_track_name;
             else info.track_name = "UnknownTrack";
             
             info.driver_name = "User";
             
             // Snapshot critical FFB settings
             info.gain = engine.m_gain;
             info.understeer_effect = engine.m_understeer_effect;
             info.sop_effect = engine.m_sop_effect;
             info.slope_enabled = engine.m_slope_detection_enabled;
             info.slope_sensitivity = engine.m_slope_sensitivity;
             info.slope_threshold = (float)engine.m_slope_negative_threshold;
             info.slope_alpha_threshold = engine.m_slope_alpha_threshold;
             info.slope_decay_rate = engine.m_slope_decay_rate;
             
             AsyncLogger::Get().Start(info, Config::m_log_path);
         }
         ImGui::SameLine();
         ImGui::TextDisabled("(Diagnostics)");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Save Screenshot")) {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "screenshot_%Y-%m-%d_%H-%M-%S.png", &tstruct);
        SaveCompositeScreenshot(buf);
    }
    
    ImGui::Separator();

    // --- HELPER LAMBDAS ---
    static int selected_preset = 0;
    
    // val: The current slider value (0.0 - 2.0)
    // base_nm: The physical force this effect produces at Gain 1.0 (Physics Constant)
    auto FormatDecoupled = [&](float val, float base_nm) {
        float scale = (engine.m_max_torque_ref / 20.0f); 
        if (scale < 0.1f) scale = 0.1f;
        float estimated_nm = val * base_nm * scale;
        static char buf[64];
        // Use double percent (%%%%) because SliderFloat formats it again
        // Show 1 decimal to make arrow key adjustments visible (step 0.01 = 0.5%)
        snprintf(buf, 64, "%.1f%%%% (~%.1f Nm)", val * 100.0f, estimated_nm); 
        return (const char*)buf;
    };

    auto FormatPct = [&](float val) {
        static char buf[32];
        // Show 1 decimal to make arrow key adjustments visible
        snprintf(buf, 32, "%.1f%%%%", val * 100.0f);
        return (const char*)buf;
    };

    auto FloatSetting = [&](const char* label, float* v, float min, float max, const char* fmt = "%.2f", const char* tooltip = nullptr, std::function<void()> decorator = nullptr) {
        GuiWidgets::Result res = GuiWidgets::Float(label, v, min, max, fmt, tooltip, decorator);
        if (res.changed) {
            selected_preset = -1;
        }
        if (res.deactivated) {
            Config::Save(engine);
        }
    };

    auto BoolSetting = [&](const char* label, bool* v, const char* tooltip = nullptr) {
        GuiWidgets::Result res = GuiWidgets::Checkbox(label, v, tooltip);
        if (res.changed) {
            selected_preset = -1;
        }
        if (res.deactivated) {
            Config::Save(engine);
        }
    };

    auto IntSetting = [&](const char* label, int* v, const char* const items[], int items_count, const char* tooltip = nullptr) {
        GuiWidgets::Result res = GuiWidgets::Combo(label, v, items, items_count, tooltip);
        if (res.changed) {
            selected_preset = -1;
        }
        if (res.deactivated) {
            Config::Save(engine);
        }
    };

    // --- 2. PRESETS AND CONFIGURATION ---
    if (ImGui::TreeNodeEx("Presets and Configuration", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        if (Config::presets.empty()) Config::LoadPresets();

        // Initialize selected_preset from last saved name if first run
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

        static char new_preset_name[64] = "";
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
        ImGui::InputText("##NewPresetName", new_preset_name, 64);
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
        
        if (ImGui::Button("Save Current Config")) {
            if (selected_preset >= 0 && selected_preset < (int)Config::presets.size() && !Config::presets[selected_preset].is_builtin) {
                Config::AddUserPreset(Config::presets[selected_preset].name, engine);
            } else {
                Config::Save(engine);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults")) {
            Config::ApplyPreset(0, engine);
            selected_preset = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) {
            if (selected_preset >= 0) {
                Config::DuplicatePreset(selected_preset, engine);
                // Select the newly added preset
                for (int i = 0; i < (int)Config::presets.size(); i++) {
                    if (Config::presets[i].name == Config::m_last_preset_name) {
                        selected_preset = i;
                        break;
                    }
                }
            }
        }
        ImGui::SameLine();
        bool can_delete = (selected_preset >= 0 && selected_preset < (int)Config::presets.size() && !Config::presets[selected_preset].is_builtin);
        if (!can_delete) ImGui::BeginDisabled();
        if (ImGui::Button("Delete")) {
            Config::DeletePreset(selected_preset, engine);
            selected_preset = 0;
            Config::ApplyPreset(0, engine);
        }
        if (!can_delete) ImGui::EndDisabled();

        ImGui::Separator();
        if (ImGui::Button("Import Preset...")) {
            std::string path;
            if (OpenPresetFileDialog(g_hwnd, path)) {
                if (Config::ImportPreset(path, engine)) {
                    // Success! The new preset is at the end of the list
                    selected_preset = (int)Config::presets.size() - 1;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Selected...")) {
            if (selected_preset >= 0 && selected_preset < Config::presets.size()) {
                std::string path;
                std::string defaultName = Config::presets[selected_preset].name + ".ini";
                if (SavePresetFileDialog(g_hwnd, path, defaultName)) {
                    Config::ExportPreset(selected_preset, path);
                }
            }
        }

        ImGui::TreePop();
    }

    ImGui::Spacing();

    // --- 3. MAIN SETTINGS GRID ---
    ImGui::Columns(2, "SettingsGrid", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);

    // --- GROUP: GENERAL ---
    if (ImGui::TreeNodeEx("General FFB", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        BoolSetting("Invert FFB Signal", &engine.m_invert_force, "Check this if the wheel pulls away from center instead of aligning.");
        FloatSetting("Master Gain", &engine.m_gain, 0.0f, 2.0f, FormatPct(engine.m_gain), "Global scale factor for all forces.\n100% = No attenuation.\nReduce if experiencing heavy clipping.");
        FloatSetting("Max Torque Ref", &engine.m_max_torque_ref, 1.0f, 200.0f, "%.1f Nm", "The expected PEAK torque of the CAR in the game.\nGT3/LMP2 cars produce 30-60 Nm of torque.\nSet this to ~40-60 Nm to prevent clipping.\nHigher values = Less Clipping, Less Noise, Lighter Steering.\nLower values = More Clipping, More Noise, Heavier Steering.");
        FloatSetting("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f", "Boosts small forces to overcome the mechanical friction/deadzone of gear/belt driven wheels.\nPrevents the 'dead center' feeling.\nTypical: 0.0 for DD, 0.01-0.05 for Belt/Gear.");
        
        ImGui::TreePop();
    } else { 
        // Keep columns synchronized when section is collapsed
        ImGui::NextColumn(); ImGui::NextColumn(); 
    }

    // --- GROUP: FRONT AXLE ---
    if (ImGui::TreeNodeEx("Front Axle (Understeer)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        FloatSetting("Steering Shaft Gain", &engine.m_steering_shaft_gain, 0.0f, 2.0f, FormatPct(engine.m_steering_shaft_gain), "Scales the raw steering torque from the physics engine.\n100% = 1:1 with game physics.\nLowering this allows other effects (SoP, Vibes) to stand out more without clipping.");
        
        FloatSetting("Steering Shaft Smoothing", &engine.m_steering_shaft_smoothing, 0.000f, 0.100f, "%.3f s", 
            "Low Pass Filter applied ONLY to the raw game force (Steering Shaft Gain).\nSmoothes out grainy or noisy signals from the game engine.",
            [&]() {
                int ms = (int)(engine.m_steering_shaft_smoothing * 1000.0f + 0.5f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        // Display with percentage format for better clarity
        FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 2.0f, FormatPct(engine.m_understeer_effect), 
            "Scales how much front grip loss reduces steering force.\n\n"
            "SCALE:\n"
            "  0% = Disabled (no understeer feel)\n"
            "  50% = Subtle (half of grip loss reflected)\n"
            "  100% = Normal (force matches grip) [RECOMMENDED]\n"
            "  200% = Maximum (very light wheel on any slide)\n\n"
            "If wheel feels too light at the limit:\n"
            "  → First INCREASE 'Optimal Slip Angle' setting in the Physics section.\n"
            "  → Then reduce this slider if still too sensitive.\n\n"
            "Technical: Force = Base * (1.0 - GripLoss * Effect)");
        
        const char* base_modes[] = { "Native (Steering Shaft Torque)", "Synthetic (Constant)", "Muted (Off)" };
        IntSetting("Base Force Mode", &engine.m_base_force_mode, base_modes, sizeof(base_modes)/sizeof(base_modes[0]), "Debug tool to isolate effects.\nNative: Normal Operation.\nSynthetic: Constant force to test direction.\nMuted: Disables base physics (good for tuning vibrations).");

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
            // Keep columns synchronized when section is collapsed
            ImGui::NextColumn(); ImGui::NextColumn();
        }
        
        ImGui::TreePop();
    } else { 
        // Keep columns synchronized when section is collapsed
        ImGui::NextColumn(); ImGui::NextColumn(); 
    }

    // --- GROUP: REAR AXLE ---
    if (ImGui::TreeNodeEx("Rear Axle (Oversteer)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        FloatSetting("Lateral G Boost (Slide)", &engine.m_oversteer_boost, 0.0f, 4.0f, FormatPct(engine.m_oversteer_boost), "Increases the Lateral G (SoP) force when the rear tires lose grip.\nMakes the car feel heavier during a slide, helping you judge the momentum.\nShould build up slightly more gradually than Rear Align Torque,\nreflecting the inertia of the car's mass swinging out.\nIt's a sustained force that tells you about the magnitude of the slide\nTuning Goal: The driver should feel the direction of the counter-steer (Rear Align)\nand the effort required to hold it (Lateral G Boost).");
        FloatSetting("Lateral G", &engine.m_sop_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_sop_effect, FFBEngine::BASE_NM_SOP_LATERAL), "Represents Chassis Roll, simulates the weight of the car leaning in the corner.");
        FloatSetting("SoP Self-Aligning Torque", &engine.m_rear_align_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_rear_align_effect, FFBEngine::BASE_NM_REAR_ALIGN), "Counter-steering force generated by rear tire slip.\nShould build up very quickly after the Yaw Kick, as the slip angle develops.\nThis is the active \"pull.\"\nTuning Goal: The driver should feel the direction of the counter-steer (Rear Align)\nand the effort required to hold it (Lateral G Boost).");
        FloatSetting("Yaw Kick", &engine.m_sop_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_sop_yaw_gain, FFBEngine::BASE_NM_YAW_KICK), "This is the earliest cue for rear stepping out. It's a sharp, momentary impulse that signals the onset of rotation.\nBased on Yaw Acceleration.");
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

        // SoP Smoothing with Latency Text above slider
        FloatSetting("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f", 
            "Filters the Lateral G signal.\nReduces jerkiness in the SoP effect.",
            [&]() {
                int ms = (int)((1.0f - engine.m_sop_smoothing_factor) * 100.0f + 0.5f);
                ImVec4 color = (ms < LATENCY_WARNING_THRESHOLD_MS) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms - %s", ms, (ms < LATENCY_WARNING_THRESHOLD_MS) ? "OK" : "High");
            });

        FloatSetting("  SoP Scale", &engine.m_sop_scale, 0.0f, 20.0f, "%.2f", "Multiplies the raw G-force signal before limiting.\nAdjusts the dynamic range of the SoP effect.");
        
        ImGui::TreePop();
    } else { 
        // Keep columns synchronized when section is collapsed
        ImGui::NextColumn(); ImGui::NextColumn(); 
    }

    // --- GROUP: PHYSICS ---
    if (ImGui::TreeNodeEx("Grip & Slip Angle Estimation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        // Slip Smoothing with Latency Text above slider
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
        
        // --- SLOPE DETECTION (v0.7.0) ---
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Slope Detection (Experimental)");
        ImGui::NextColumn(); ImGui::NextColumn();

        // Slope Detection Enable - with buffer reset on transition (v0.7.0)
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
            selected_preset = -1;
            
            // Reset buffers when enabling slope detection (v0.7.0 - Prevents stale data)
            if (!prev_slope_enabled && engine.m_slope_detection_enabled) {
                engine.m_slope_buffer_count = 0;
                engine.m_slope_buffer_index = 0;
                engine.m_slope_smoothed_output = 1.0;  // Start at full grip
                std::cout << "[SlopeDetection] Enabled - buffers cleared" << std::endl;
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
            // Filter Window
            int window = engine.m_slope_sg_window;
            if (ImGui::SliderInt("  Filter Window", &window, 5, 41)) {
                if (window % 2 == 0) window++;  // Force odd
                engine.m_slope_sg_window = window;
                selected_preset = -1;
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
            
            // Advanced (Collapsed by Default)
            if (ImGui::TreeNode("Advanced Slope Settings")) {
                ImGui::NextColumn(); ImGui::NextColumn();
                FloatSetting("  Slope Threshold", &engine.m_slope_negative_threshold, -1.0f, 0.0f, "%.2f",
                    "Slope value below which grip loss begins.\n"
                    "More negative = Later detection (safer).");
                FloatSetting("  Output Smoothing", &engine.m_slope_smoothing_tau, 0.005f, 0.100f, "%.3f s",
                    "Time constant for grip factor smoothing.\n"
                    "Prevents abrupt FFB changes.");
                
                // v0.7.3: Stability Fixes
                ImGui::Separator();
                ImGui::Text("Stability Fixes (v0.7.3)");
                ImGui::NextColumn(); ImGui::NextColumn();
                FloatSetting("  Alpha Threshold", &engine.m_slope_alpha_threshold, 0.001f, 0.100f, "%.3f",
                    "Minimum change in slip angle (dAlpha/dt) to calculate slope.\n"
                    "Larger = More stable on straights, but slower response.\n"
                    "Default: 0.020");
                FloatSetting("  Decay Rate", &engine.m_slope_decay_rate, 0.5f, 20.0f, "%.1f",
                    "How fast the slope returns to zero when driving straight.\n"
                    "Prevents 'sticky' understeer feel after a corner.\n"
                    "Default: 5.0");
                BoolSetting("  Confidence Gate", &engine.m_slope_confidence_enabled,
                    "Scales the grip loss effect by how 'certain' the calculation is.\n"
                    "Uses dAlpha/dt to determine confidence.\n"
                    "Prevents random FFB jolts when slip is low.");
                
                ImGui::TreePop();
            } else {
                ImGui::NextColumn(); ImGui::NextColumn();
            }
            
            // Live Diagnostics
            ImGui::Text("  Live Slope: %.3f | Grip: %.0f%%", 
                engine.m_slope_current, 
                engine.m_slope_smoothed_output * 100.0f);
            ImGui::NextColumn(); ImGui::NextColumn();
        }
        // ---------------------------------

        
        
        ImGui::TreePop();
    } else { 
        // Keep columns synchronized when section is collapsed
        ImGui::NextColumn(); ImGui::NextColumn(); 
    }

    // --- GROUP: BRAKING & LOCKUP ---
    if (ImGui::TreeNodeEx("Braking & Lockup", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();

        BoolSetting("Lockup Vibration", &engine.m_lockup_enabled, "Simulates tire judder when wheels are locked under braking.");
        if (engine.m_lockup_enabled) {
            FloatSetting("  Lockup Strength", &engine.m_lockup_gain, 0.0f, 3.0f, FormatDecoupled(engine.m_lockup_gain, FFBEngine::BASE_NM_LOCKUP_VIBRATION));
            FloatSetting("  Brake Load Cap", &engine.m_brake_load_cap, 1.0f, 10.0f, "%.2fx", "Scales vibration intensity based on tire load.\nPrevents weak vibrations during high-speed heavy braking.");
            
            FloatSetting("  Vibration Pitch", &engine.m_lockup_freq_scale, 0.5f, 2.0f, "%.2fx", "Scales the frequency of lockup and wheel spin vibrations.\nMatch to your hardware resonance.");
            
            
            // Precision formatting rationale (v0.6.0):
            // - Gamma: %.1f (1 decimal) - Allows fine-tuning of response curve
            // - Sensitivity: %.0f (0 decimals) - Integer values are sufficient for threshold
            // - Bump Rejection: %.1f m/s (1 decimal) - Balances precision with readability
            // - ABS Gain: %.2f (2 decimals) - Standard gain precision across all effects
            ImGui::Separator();
            ImGui::Text("Response Curve");
            ImGui::NextColumn(); ImGui::NextColumn();

            FloatSetting("  Gamma", &engine.m_lockup_gamma, 0.1f, 3.0f, "%.1f", "Response Curve Non-Linearity.\n1.0 = Linear.\n>1.0 = Progressive (Starts weak, gets strong fast).\n<1.0 = Aggressive (Starts strong). 2.0=Quadratic, 3.0=Cubic (Late/Sharp)");
            FloatSetting("  Start Slip %", &engine.m_lockup_start_pct, 1.0f, 10.0f, "%.1f%%", "Slip percentage where vibration begins.\n1.0% = Immediate feedback.\n5.0% = Only on deep lock.");
            FloatSetting("  Full Slip %", &engine.m_lockup_full_pct, 5.0f, 25.0f, "%.1f%%", "Slip percentage where vibration reaches maximum intensity.");
            
            
            // Precision formatting rationale (v0.6.0):
            // - Gamma: %.1f (1 decimal) - Allows fine-tuning of response curve
            // - Sensitivity: %.0f (0 decimals) - Integer values are sufficient for threshold
            // - Bump Rejection: %.1f m/s (1 decimal) - Balances precision with readability
            // - ABS Gain: %.2f (2 decimals) - Standard gain precision across all effects
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

        // ABS
        BoolSetting("ABS Pulse", &engine.m_abs_pulse_enabled, "Simulates the pulsing of an ABS system.\nInjects high-frequency pulse when ABS modulates pressure.");
        if (engine.m_abs_pulse_enabled) {
            FloatSetting("  Pulse Gain", &engine.m_abs_gain, 0.0f, 10.0f, "%.2f", "Intensity of the ABS pulse.");
            FloatSetting("  Pulse Frequency", &engine.m_abs_freq_hz, 10.0f, 50.0f, "%.1f Hz", "Rate of the ABS pulse oscillation.");
        }

        ImGui::TreePop();
    } else {
        ImGui::NextColumn(); ImGui::NextColumn();
    }

    // --- GROUP: TEXTURES ---
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
        // Keep columns synchronized when section is collapsed
        ImGui::NextColumn(); ImGui::NextColumn(); 
    }

    // --- ADVANCED SETTINGS ---
    if (ImGui::CollapsingHeader("Advanced Settings")) {
        ImGui::Indent();
        
        if (ImGui::TreeNode("Stationary Vibration Gate")) {
            ImGui::TextWrapped("Controls when vibrations fade out and Idle Smoothing activates.");
            
            float lower_kmh = engine.m_speed_gate_lower * 3.6f;
            if (ImGui::SliderFloat("Mute Below", &lower_kmh, 0.0f, 20.0f, "%.1f km/h")) {
                engine.m_speed_gate_lower = lower_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f) 
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
                selected_preset = -1;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Config::Save(engine);
            }

            float upper_kmh = engine.m_speed_gate_upper * 3.6f;
            if (ImGui::SliderFloat("Full Above", &upper_kmh, 1.0f, 50.0f, "%.1f km/h")) {
                engine.m_speed_gate_upper = upper_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f)
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
                selected_preset = -1;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Config::Save(engine);
            }
            
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(
                "Speed where vibrations reach full strength.\n"
                "CRITICAL: Speeds below this value will have SMOOTHING applied\n"
                "to eliminate engine idle vibration.\n"
                "Default: 18.0 km/h (Safe for all wheels).");
            
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Telemetry Logger")) {
            if (ImGui::Checkbox("Auto-Start on Session", &Config::m_auto_start_logging)) {
                Config::Save(engine);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Automatically start logging when you leave the pit/menu,\nand stop when you return.");
            
            char log_path[260];
            strncpy_s(log_path, Config::m_log_path.c_str(), _TRUNCATE);
            if (ImGui::InputText("Log Path", log_path, sizeof(log_path))) {
                Config::m_log_path = log_path;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Directory where log files will be saved.\nRelative to app root if no drive letter.\nExample: logs/");

            if (AsyncLogger::Get().IsLogging()) {
                ImGui::BulletText("Filename: %s", AsyncLogger::Get().GetFilename().c_str());
            }

            char log_path_buf[256];
            strncpy_s(log_path_buf, Config::m_log_path.c_str(), 255);
            log_path_buf[255] = '\0';
            if (ImGui::InputText("Log Path", log_path_buf, 255)) {
                Config::m_log_path = log_path_buf;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Config::Save(engine);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Directory where .csv logs will be saved.\nDefault: logs/");
            
            ImGui::TreePop();
        }
        ImGui::Unindent();
    }

    // End Columns
    ImGui::Columns(1);
    
    ImGui::End();
}

// --- FILE DIALOG HELPERS ---

bool OpenPresetFileDialog(HWND hwnd, std::string& outPath) {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
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

bool SavePresetFileDialog(HWND hwnd, std::string& outPath, const std::string& defaultName) {
    char filename[MAX_PATH] = "";
    strncpy_s(filename, defaultName.c_str(), _TRUNCATE);

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
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

// Helper to toggle Topmost
void SetWindowAlwaysOnTop(HWND hwnd, bool enabled) {
    if (!hwnd) return;
    HWND insertAfter = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
    // SWP_NOMOVE | SWP_NOSIZE means we only change Z-order, not position/size
    // SWP_NOACTIVATE prevents stealing focus, SWP_FRAMECHANGED ensures style bits are refreshed
    ::SetWindowPos(hwnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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
const float PLOT_HISTORY_SEC = 30.0f;   // 30 Seconds History
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
static RollingBuffer plot_slope_current;  // New v0.7.1: Slope detection diagnostic
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
static bool g_warn_dt = false;

// Toggle State
// Redundant variable removed (using Config::show_graphs)

void GuiLayer::DrawDebugWindow(FFBEngine& engine) {
    // Only draw if enabled
    if (!Config::show_graphs) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport(); 

    // Position: Start after the config panel
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + CONFIG_PANEL_WIDTH, viewport->Pos.y));
    
    // Size: Fill the rest of the width
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - CONFIG_PANEL_WIDTH, viewport->Size.y));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("FFB Analysis", nullptr, flags);

    // Ensure snapshots are processed
    // (Existing snapshot processing logic follows)
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
        plot_slope_current.Add(snap.slope_current); // v0.7.1

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
        g_warn_dt = snap.warn_dt;
    }

    // --- Draw Warnings ---
    if (g_warn_dt) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("TELEMETRY WARNINGS:");
        ImGui::Text("- Invalid DeltaTime (Using 400Hz fallback)");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // --- Header A: FFB Components (Output) ---
    // [Main Forces], [Modifiers], [Textures]
    if (ImGui::CollapsingHeader("A. FFB Components (Output)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);        
        //ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Main FFB]");

        ImGui::Text("Throttle/Brake Input");
        float thr = plot_raw_throttle.GetCurrent();
        float brk = plot_raw_brake.GetCurrent();
        char input_label[128];
        snprintf(input_label, sizeof(input_label), "Thr: %.2f | Brk: %.2f", thr, brk);
        ImGui::SameLine(); 
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", input_label);
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
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
        PlotWithStats("Total Output", plot_total, -1.0f, 1.0f, ImVec2(0, 60), 
                      "Final FFB Output (-1.0 to 1.0)");
        ImGui::PopStyleColor();
                      
        ImGui::Columns(2, "FFBMain", false);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.25f, 0.25f, 0.25f, 0.50f)); // Red
        PlotWithStats("Clipping", plot_clipping, 0.0f, 1.1f, ImVec2(0, 40),
            "Indicates when Output hits max limit");
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 0.7f, 1.0f, 1.0f)); // Blue
        PlotWithStats("Base Torque (Nm)", plot_base, -30.0f, 30.0f, ImVec2(0, 40),
            "Steering Rack Force derived from Game Physics");
        ImGui::PopStyleColor();

        ImGui::Columns(3, "SOPMain", false);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[SoP]");
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); //orange
        PlotWithStats("SoP (Base Chassis G)", plot_sop, -20.0f, 20.0f, ImVec2(0, 40),
            "Force from Lateral G-Force (Seat of Pants)");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); //orange
        PlotWithStats("Yaw Kick", plot_yaw_kick, -20.0f, 20.0f, ImVec2(0, 40),
            "Force from Yaw Acceleration (Rotation Kick)");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); //orange
        PlotWithStats("Rear Align Torque", plot_rear_torque, -20.0f, 20.0f, ImVec2(0, 40),
            "Force from Rear Lateral Force");
        ImGui::PopStyleColor();
        
        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        
        // Group: Modifiers
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[Modifiers]");
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); //orange
        
        PlotWithStats("Lateral G Boost (Slide)", plot_oversteer, -20.0f, 20.0f, ImVec2(0, 40),
                      "Added force from Rear Grip loss");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); //orange        
        PlotWithStats("Understeer Cut", plot_understeer, -20.0f, 20.0f, ImVec2(0, 40),
                      "Reduction in force due to front grip loss");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); //orange   
        PlotWithStats("Gyro Damping", plot_gyro_damping, -20.0f, 20.0f, ImVec2(0, 40),
                    "Synthetic damping force");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); //orange   
        PlotWithStats("Scrub Drag Force", plot_scrub_drag, -20.0f, 20.0f, ImVec2(0, 40),
                    "Resistance force from sideways tire dragging");
        ImGui::PopStyleColor();
        
        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        
        // Group: Textures
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "[Textures]");
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 1.0f, 0.7f, 1.0f)); // Green
        PlotWithStats("Road Texture", plot_road, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Suspension Velocity");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 1.0f, 0.7f, 1.0f)); // Green
        PlotWithStats("Slide Texture", plot_slide, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Lateral Scrubbing");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 1.0f, 0.7f, 1.0f)); // Green
        PlotWithStats("Lockup Vib", plot_lockup, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Wheel Lockup");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 1.0f, 0.7f, 1.0f)); // Green
        PlotWithStats("Spin Vib", plot_spin, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Wheel Spin");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 1.0f, 0.7f, 1.0f)); // Green
        PlotWithStats("Bottoming", plot_bottoming, -10.0f, 10.0f, ImVec2(0, 40),
                      "Vibration from Suspension Bottoming");
        ImGui::PopStyleColor();

        ImGui::Columns(1);
    }

    // --- Header B: Internal Physics (Brain) ---
    // [Loads], [Grip/Slip], [Forces]
    if (ImGui::CollapsingHeader("B. Internal Physics (Brain)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(2, "PhysCols", false);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        
        // Group: Loads
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Loads/Forces]");
        
        // --- Manually draw stats for the Multi-line Load Graph ---
        float cur_f = plot_calc_front_load.GetCurrent();
        float cur_r = plot_calc_rear_load.GetCurrent();
        char load_label[128];
        snprintf(load_label, sizeof(load_label), "Front: %.0f N | Rear: %.0f N", cur_f, cur_r);
        ImGui::Text("%s", load_label);
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadF", plot_calc_front_load.data.data(), (int)plot_calc_front_load.data.size(), plot_calc_front_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor();
        ImVec2 pos_load = ImGui::GetItemRectMin(); // Reset Cursor to draw on top
        ImGui::SetCursorScreenPos(pos_load);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0)); // Draw Rear (Magenta) - Transparent Background
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
        ImGui::PlotLines("##CLoadR", plot_calc_rear_load.data.data(), (int)plot_calc_rear_load.data.size(), plot_calc_rear_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
        ImGui::PopStyleColor(2);        
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load:\nCyan: Front\nMagenta: Rear");
        
        // Group: Forces
        //ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Forces]");
        
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));  //Magenta
        PlotWithStats("Calc Rear Lat Force", plot_calc_rear_lat_force, -5000.0f, 5000.0f, ImVec2(0, 40),
                      "Calculated Rear Lateral Force (Workaround)");
        ImGui::PopStyleColor();

        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        
        // Group: Grip/Slip
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Grip/Slip]");
        
        //PlotWithStats("Calc Front Grip", plot_calc_front_grip, 0.0f, 1.2f, ImVec2(0, 40),
        //              "Grip used for physics math (approximated if missing)");
        
        //PlotWithStats("Calc Rear Grip", plot_calc_rear_grip, 0.0f, 1.2f, ImVec2(0, 40),
        //              "Rear Grip used for SoP/Oversteer math");
        
        // --- Manually draw stats for the Multi-line Load Graph ---
        {
            float curg_f = plot_calc_front_grip.GetCurrent();
            float curg_r = plot_calc_rear_grip.GetCurrent();
            char grip_label[128];
            snprintf(grip_label, sizeof(grip_label), "Grip Front: %.0f | Rear: %.0f ", curg_f, curg_r);
            ImGui::Text("%s", grip_label);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##CGripF", plot_calc_front_grip.data.data(), (int)plot_calc_front_grip.data.size(), plot_calc_front_grip.offset, NULL, -0.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor();
            ImVec2 pos_grip = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(pos_grip);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##GripR", plot_calc_rear_grip.data.data(), (int)plot_calc_rear_grip.data.size(), plot_calc_rear_grip.offset, NULL, -0.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Calc Grip\nCyan: Front\nMagenta: Rear");
        }
        
        //PlotWithStats("Front Slip Angle (Sm)", plot_calc_slip_angle_smoothed, 0.0f, 1.0f, ImVec2(0, 40),
        //              "Smoothed Slip Angle (LPF) used for approximation");
        
        //PlotWithStats("Rear Slip Angle (Sm)", plot_calc_rear_slip_angle_smoothed, 0.0f, 1.0f, ImVec2(0, 40),
        //              "Smoothed Rear Slip Angle (LPF)");
        
        // --- Manually draw stats for the Multi-line Load Graph ---
        {
            float curs_f = plot_calc_slip_angle_smoothed.GetCurrent();
            float curs_r = plot_calc_rear_slip_angle_smoothed.GetCurrent();
            char slip_label[128];
            snprintf(slip_label, sizeof(slip_label), "Slip Angle Front: %.0f | Rear: %.0f ", curs_f, curs_r);
            ImGui::Text("%s", slip_label);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##CSlipF", plot_calc_slip_angle_smoothed.data.data(), (int)plot_calc_slip_angle_smoothed.data.size(), plot_calc_slip_angle_smoothed.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
            ImGui::PopStyleColor();
            ImVec2 pos_slip = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(pos_slip);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##SlipR", plot_calc_rear_slip_angle_smoothed.data.data(), (int)plot_calc_rear_slip_angle_smoothed.data.size(), plot_calc_rear_slip_angle_smoothed.offset, NULL, 0.0f, 1.0f, ImVec2(0, 40));
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Slip Angle Cyan: Front, Magenta: Rear");
        }

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));  //Magenta
        PlotWithStats("Front Slip Ratio", plot_calc_slip_ratio, -1.0f, 1.0f, ImVec2(0, 40),
                      "Calculated or Game-provided Slip Ratio");
        ImGui::PopStyleColor();

        if (engine.m_slope_detection_enabled) { 
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));  //Yellow
            PlotWithStats("Slope (dG/dAlpha)", plot_slope_current, -5.0f, 5.0f, ImVec2(0, 40),
                "Slope detection derivative value.\n"
                "Positive = building grip.\n"
                "Near zero = at peak grip.\n"
                "Negative = past peak, sliding.");
                ImGui::PopStyleColor();
        }
        
        ImGui::Columns(1);
    }

    // --- Header C: Raw Game Telemetry (Input) ---
    // [Driver Input], [Vehicle State], [Raw Tire Data], [Patch Velocities]
    if (ImGui::CollapsingHeader("C. Raw Game Telemetry (Input)", ImGuiTreeNodeFlags_None)) {
        ImGui::Columns(4, "TelCols", false);
        
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);

/**
        // Group: Driver Input
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Driver Input]");
        if (ImPlot::BeginPlot("Combined Input", ImVec2(0, 150)))
        {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
            // Steering: -1..1, Brake/Throttle: 0..1
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImGuiCond_Always);

            const int n = (int)plot_raw_brake.data.size();
            ImPlot::PlotLine("Brake",    plot_raw_brake.data.data(),          n, 1.0, 0.0, 0, plot_raw_brake.offset);
            ImPlot::PlotLine("Throttle", plot_raw_throttle.data.data(),       n, 1.0, 0.0, 0, plot_raw_throttle.offset);
            ImPlot::PlotLine("Steering", plot_raw_input_steering.data.data(), n, 1.0, 0.0, 0, plot_raw_input_steering.offset);

            ImPlot::EndPlot();
        }
**/


        
            
       
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Driver Input]");
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
            PlotWithStats("Steering Torque", plot_raw_steer, -30.0f, 30.0f, ImVec2(0, 40),
                        "Raw Steering Torque from Game API");
            ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
            PlotWithStats("Steering Input", plot_raw_input_steering, -1.0f, 1.0f, ImVec2(0, 40),
                        "Driver wheel position -1 to 1");
            ImGui::PopStyleColor();
                    
        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        
        // Group: Vehicle State
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Vehicle State]");

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));        
        PlotWithStats("Chassis Lat Accel", plot_input_accel, -20.0f, 20.0f, ImVec2(0, 40),
                      "Local Lateral Acceleration (G)");
                      ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));        
        PlotWithStats("Car Speed (m/s)", plot_raw_car_speed, 0.0f, 100.0f, ImVec2(0, 40),
                      "Vehicle Speed");
                      ImGui::PopStyleColor();
        
        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        
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
            
            ImGui::Text("%s", stats_label);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));  //Magenta
            ImGui::PlotLines("##RawLoad", plot_raw_load.data.data(), (int)plot_raw_load.data.size(), 
                           plot_raw_load.offset, NULL, 0.0f, 10000.0f, ImVec2(0, 40));
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raw Tire Load from Game API");
        }
        
        // Raw Front Grip with warning label and stats
        //{
        //    float current = plot_raw_grip.GetCurrent();
        //    float min_val = plot_raw_grip.GetMin();
        //    float max_val = plot_raw_grip.GetMax();
        //    char stats_label[256];
        //    snprintf(stats_label, sizeof(stats_label), "Front Grip | Val: %.4f | Min: %.3f | Max: %.3f",
        //        current, min_val, max_val);
        //    ImGui::Text("%s", stats_label);
        //    ImGui::PlotLines("##RawGrip", plot_raw_grip.data.data(), (int)plot_raw_grip.data.size(),
        //        plot_raw_grip.offset, NULL, 0.0f, 1.2f, ImVec2(0, 40));
        //    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raw Grip Fraction from Game API");
        //}
        
        //PlotWithStats("Raw Rear Grip", plot_raw_rear_grip, 0.0f, 1.2f, ImVec2(0, 40),
        //    "Raw Rear Grip Fraction from Game API");


        {
            float curgr_f = plot_raw_grip.GetCurrent();
            float curgr_r = plot_raw_rear_grip.GetCurrent();
            char gr_label[128];
            snprintf(gr_label, sizeof(gr_label), "Grip Fraction Front: %.0f | Rear: %.0f ", curgr_f, curgr_r);
            ImGui::Text("%s", gr_label);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##GrippF", plot_raw_grip.data.data(), (int)plot_raw_grip.data.size(), plot_raw_grip.offset, NULL, -0.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor();
            ImVec2 pos_load = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(pos_load);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##GrippR", plot_raw_rear_grip.data.data(), (int)plot_raw_rear_grip.data.size(), plot_raw_rear_grip.offset, NULL, -0.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Raw Grip Fraction\nCyan: Front\nMagenta: Rear");
        }



        ImGui::NextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);
        
        // Group: Patch Velocities
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[Patch Velocities]");
        //PlotWithStats("Avg Front Lat PatchVel", plot_raw_front_lat_patch_vel, 0.0f, 20.0f, ImVec2(0, 40),
        //    "Lateral Velocity at Contact Patch");
        //PlotWithStats("Avg Rear Lat PatchVel", plot_raw_rear_lat_patch_vel, 0.0f, 20.0f, ImVec2(0, 40),
        //    "Lateral Velocity at Contact Patch (Rear)");

        {
            float curlp_f = plot_raw_front_lat_patch_vel.GetCurrent();
            float curlp_r = plot_raw_rear_lat_patch_vel.GetCurrent();
            char lat_label[128];
            snprintf(lat_label, sizeof(lat_label), "LatVel Patch Front: %.0f | Rear: %.0f ", curlp_f, curlp_r);
            ImGui::Text("%s", lat_label);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##LatF", plot_raw_front_lat_patch_vel.data.data(), (int)plot_raw_front_lat_patch_vel.data.size(), plot_raw_front_lat_patch_vel.offset, NULL, -0.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor();
            ImVec2 pos_load = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(pos_load);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##LatR", plot_raw_rear_lat_patch_vel.data.data(), (int)plot_raw_rear_lat_patch_vel.data.size(), plot_raw_rear_lat_patch_vel.offset, NULL, -0.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lateral Velocity at Contact Patch Cyan: Front, Magenta: Rear");
        }


        //PlotWithStats("Avg Front Long PatchVel", plot_raw_front_long_patch_vel, -20.0f, 20.0f, ImVec2(0, 40),
        //    "Longitudinal Velocity at Contact Patch (Front)");
        //PlotWithStats("Avg Rear Long PatchVel", plot_raw_rear_long_patch_vel, -20.0f, 20.0f, ImVec2(0, 40),
        //    "Longitudinal Velocity at Contact Patch (Rear)");

        {
            float curllp_f = plot_raw_front_long_patch_vel.GetCurrent();
            float curllp_r = plot_raw_rear_long_patch_vel.GetCurrent();
            char llong_label[128];
            snprintf(llong_label, sizeof(llong_label), "LongVel ContPatch Front: %.0f | Rear: %.0f ", curllp_f, curllp_r);
            ImGui::Text("%s", llong_label);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##LlongF", plot_raw_front_long_patch_vel.data.data(), (int)plot_raw_front_long_patch_vel.data.size(), plot_raw_front_long_patch_vel.offset, NULL, -1.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor();
            ImVec2 pos_load = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(pos_load);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
            ImGui::PlotLines("##LlongR", plot_raw_rear_long_patch_vel.data.data(), (int)plot_raw_rear_long_patch_vel.data.size(), plot_raw_rear_long_patch_vel.offset, NULL, -1.2f, 1.2f, ImVec2(0, 40));
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Longitudinal Velocity at Contact Patch Cyan: Front, Magenta: Rear");
        }

        ImGui::Columns(1);
    }

    ImGui::End();
}
