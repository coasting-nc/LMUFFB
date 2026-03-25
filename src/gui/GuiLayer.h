#ifndef GUILAYER_H
#define GUILAYER_H

#include "FFBEngine.h"
#include <string>

class GuiLayer {
public:
    friend class GuiLayerTestAccess;
    static bool Init();
    static void Shutdown(LMUFFB::FFBEngine& engine);
    
    static void* GetWindowHandle(); // Returns HWND on Windows, GLFWwindow* on Linux
    static void SetupGUIStyle();   // Setup professional theme

    // Returns true if the GUI is active/focused (affects lazy rendering)
    static bool Render(LMUFFB::FFBEngine& engine);

    // Pulls latest snapshots from the engine and updates GUI state
    static void UpdateTelemetry(LMUFFB::FFBEngine& engine);

    // Snapshot state for thread-safe UI display
    static float GetLatestSteeringRange() { return m_latest_steering_range; }
    static float GetLatestSteeringAngle() { return m_latest_steering_angle; }

private:
    static float m_latest_steering_range;
    static float m_latest_steering_angle;

    static void DrawMenuBar(LMUFFB::FFBEngine& engine);
    static void LaunchLogAnalyzer(const std::string& log_file);
    static void DrawTuningWindow(LMUFFB::FFBEngine& engine);
    static void DrawDebugWindow(LMUFFB::FFBEngine& engine);
};

// Platform helper functions (implemented in GuiLayer_Win32.cpp and GuiLayer_Linux.cpp)
void ResizeWindowPlatform(int x, int y, int w, int h);
void SaveCurrentWindowGeometryPlatform(bool is_graph_mode);
void SetWindowAlwaysOnTopPlatform(bool enabled);
bool OpenPresetFileDialogPlatform(std::string& outPath);
bool SavePresetFileDialogPlatform(std::string& outPath, const std::string& defaultName);

#endif // GUILAYER_H
