#ifndef GUILAYER_H
#define GUILAYER_H

#include "FFBEngine.h"

// Forward declaration to avoid pulling in ImGui headers here if not needed
struct GuiContext; 

class GuiLayer {
public:
    static bool Init();
    static void Shutdown(FFBEngine& engine);
    
    static void* GetWindowHandle(); // Returns HWND
    static void SetupGUIStyle();   // Setup professional "Deep Dark" theme

    // Returns true if the GUI is active/focused (affects lazy rendering)
    static bool Render(FFBEngine& engine);

private:
    static void DrawTuningWindow(FFBEngine& engine);
    static void DrawDebugWindow(FFBEngine& engine);
    
    // UI State (Persistent state managed via Config::show_graphs)
    // Note: Removed redundant GuiLayer::m_show_debug_window static variable in v0.5.5
    // to consolidate state management in Config class for better persistence across sessions
};

#endif // GUILAYER_H
