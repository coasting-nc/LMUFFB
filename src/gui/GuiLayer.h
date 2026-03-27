#ifndef GUILAYER_H
#define GUILAYER_H

#include "FFBEngine.h"
#include <string>

class GuiLayerTestAccess;

namespace LMUFFB {
namespace GUI {

class GuiLayer {
public:
    friend class ::GuiLayerTestAccess;
    static bool Init();
    static void Shutdown(LMUFFB::FFB::FFBEngine& engine);
    
    static void* GetWindowHandle(); // Returns HWND on Windows, GLFWwindow* on Linux
    static void SetupGUIStyle();   // Setup professional theme

    // Returns true if the GUI is active/focused (affects lazy rendering)
    static bool Render(LMUFFB::FFB::FFBEngine& engine);

    // Pulls latest snapshots from the engine and updates GUI state
    static void UpdateTelemetry(LMUFFB::FFB::FFBEngine& engine);

    // Snapshot state for thread-safe UI display
    static float GetLatestSteeringRange() { return m_latest_steering_range; }
    static float GetLatestSteeringAngle() { return m_latest_steering_angle; }

private:
    static float m_latest_steering_range;
    static float m_latest_steering_angle;

#ifdef LMUFFB_UNIT_TEST
public:
    static std::wstring m_last_shell_execute_args;
    static std::string m_last_system_cmd;
private:
#endif

    static void DrawMenuBar(LMUFFB::FFB::FFBEngine& engine);
    static void LaunchLogAnalyzer(const std::string& log_file);
    static void DrawTuningWindow(LMUFFB::FFB::FFBEngine& engine);
    static void DrawDebugWindow(LMUFFB::FFB::FFBEngine& engine);
};

} // namespace GUI
} // namespace LMUFFB

// Platform helper functions (implemented in GuiLayer_Win32.cpp and GuiLayer_Linux.cpp)
namespace LMUFFB {
namespace GUI {
void ResizeWindowPlatform(int x, int y, int w, int h);
void SaveCurrentWindowGeometryPlatform(bool is_graph_mode);
void SetWindowAlwaysOnTopPlatform(bool enabled);
bool OpenPresetFileDialogPlatform(std::string& outPath);
bool SavePresetFileDialogPlatform(std::string& outPath, const std::string& defaultName);
}
}

#endif // GUILAYER_H
