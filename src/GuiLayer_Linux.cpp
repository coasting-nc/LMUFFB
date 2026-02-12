#include "GuiLayer.h"
#include "GuiPlatform.h"
#include "Version.h"
#include "Config.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <chrono>

#if defined(ENABLE_IMGUI) && !defined(HEADLESS_GUI)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

static GLFWwindow* g_window = nullptr;
#endif

extern std::atomic<bool> g_running;

class LinuxGuiPlatform : public IGuiPlatform {
public:
    void SetAlwaysOnTop(bool enabled) override {
#if defined(ENABLE_IMGUI) && !defined(HEADLESS_GUI)
        if (g_window) {
            glfwSetWindowAttrib(g_window, GLFW_FLOATING, enabled ? GLFW_TRUE : GLFW_FALSE);
        }
#else
        m_always_on_top_mock = enabled;
#endif
    }

    void ResizeWindow(int x, int y, int w, int h) override {
#if defined(ENABLE_IMGUI) && !defined(HEADLESS_GUI)
        if (g_window) {
            glfwSetWindowSize(g_window, w, h);
        }
#endif
    }

    void SaveWindowGeometry(bool is_graph_mode) override {
#if defined(ENABLE_IMGUI) && !defined(HEADLESS_GUI)
        if (g_window) {
            int x, y, w, h;
            glfwGetWindowPos(g_window, &x, &y);
            glfwGetWindowSize(g_window, &w, &h);
            Config::win_pos_x = x;
            Config::win_pos_y = y;
            if (is_graph_mode) {
                Config::win_w_large = w;
                Config::win_h_large = h;
            } else {
                Config::win_w_small = w;
                Config::win_h_small = h;
            }
        }
#endif
    }

    bool OpenPresetFileDialog(std::string& outPath) override {
        std::cout << "[GUI] File Dialog not implemented on Linux yet." << std::endl;
        return false;
    }

    bool SavePresetFileDialog(std::string& outPath, const std::string& defaultName) override {
        std::cout << "[GUI] File Dialog not implemented on Linux yet." << std::endl;
        return false;
    }

    void* GetWindowHandle() override {
#if defined(ENABLE_IMGUI) && !defined(HEADLESS_GUI)
        return (void*)g_window;
#else
        return nullptr;
#endif
    }

    bool GetAlwaysOnTopMock() override { return m_always_on_top_mock; }

    // Mock access for tests
    bool m_always_on_top_mock = false;
};

static LinuxGuiPlatform g_platform;
IGuiPlatform& GetGuiPlatform() { return g_platform; }

// Compatibility Helpers
void ResizeWindowPlatform(int x, int y, int w, int h) { GetGuiPlatform().ResizeWindow(x, y, w, h); }
void SaveCurrentWindowGeometryPlatform(bool is_graph_mode) { GetGuiPlatform().SaveWindowGeometry(is_graph_mode); }
void SetWindowAlwaysOnTopPlatform(bool enabled) { GetGuiPlatform().SetAlwaysOnTop(enabled); }
bool OpenPresetFileDialogPlatform(std::string& outPath) { return GetGuiPlatform().OpenPresetFileDialog(outPath); }
bool SavePresetFileDialogPlatform(std::string& outPath, const std::string& defaultName) { return GetGuiPlatform().SavePresetFileDialog(outPath, defaultName); }

#if defined(ENABLE_IMGUI) && !defined(HEADLESS_GUI)

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool GuiLayer::Init() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return false;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    int start_w = Config::show_graphs ? Config::win_w_large : Config::win_w_small;
    int start_h = Config::show_graphs ? Config::win_h_large : Config::win_h_small;

    std::string title = "lmuFFB v" + std::string(LMUFFB_VERSION);
    g_window = glfwCreateWindow(start_w, start_h, title.c_str(), NULL, NULL);
    if (g_window == NULL) return false;

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1); // Enable vsync

    if (Config::m_always_on_top) SetWindowAlwaysOnTopPlatform(true);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupGUIStyle();

    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
}

void GuiLayer::Shutdown(FFBEngine& engine) {
    SaveCurrentWindowGeometryPlatform(Config::show_graphs);
    Config::Save(engine);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(g_window);
    glfwTerminate();
}

void* GuiLayer::GetWindowHandle() {
    return (void*)g_window;
}

bool GuiLayer::Render(FFBEngine& engine) {
    if (glfwWindowShouldClose(g_window)) {
        g_running = false;
        return false;
    }

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawTuningWindow(engine);
    if (Config::show_graphs) DrawDebugWindow(engine);

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(g_window);

    return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) || ImGui::IsAnyItemActive();
}

#else
// Stub Implementation for Headless Builds (or if IMGUI disabled)
bool GuiLayer::Init() {
    std::cout << "[GUI] Disabled (Headless Mode)" << std::endl;
    return true;
}
void GuiLayer::Shutdown(FFBEngine& engine) {
    Config::Save(engine);
}
bool GuiLayer::Render(FFBEngine& engine) { return false; }
void* GuiLayer::GetWindowHandle() { return nullptr; }

#endif
