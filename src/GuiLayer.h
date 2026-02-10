#ifndef GUILAYER_H
#define GUILAYER_H

#include "FFBEngine.h"
#include <string>

class GuiLayer {
public:
    static bool Init();
    static void Shutdown(FFBEngine& engine);
    
    static void* GetWindowHandle(); // Returns HWND on Windows, GLFWwindow* on Linux
    static void SetupGUIStyle();   // Setup professional theme

    // Returns true if the GUI is active/focused (affects lazy rendering)
    static bool Render(FFBEngine& engine);

private:
    static void DrawTuningWindow(FFBEngine& engine);
    static void DrawDebugWindow(FFBEngine& engine);
};

// Platform helper functions (implemented in GuiLayer_Win32.cpp and GuiLayer_Linux.cpp)
void ResizeWindowPlatform(int x, int y, int w, int h);
void SaveCurrentWindowGeometryPlatform(bool is_graph_mode);
void SetWindowAlwaysOnTopPlatform(bool enabled);
bool OpenPresetFileDialogPlatform(std::string& outPath);
bool SavePresetFileDialogPlatform(std::string& outPath, const std::string& defaultName);

#endif // GUILAYER_H
