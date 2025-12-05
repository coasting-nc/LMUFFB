#ifndef GUILAYER_H
#define GUILAYER_H

#include "../FFBEngine.h"

// Forward declaration to avoid pulling in ImGui headers here if not needed
struct GuiContext; 

class GuiLayer {
public:
    static bool Init();
    static void Shutdown();
    
    // Returns true if the GUI is active/focused (affects lazy rendering)
    static bool Render(FFBEngine& engine);

private:
    static void DrawTuningWindow(FFBEngine& engine);
};

#endif // GUILAYER_H
