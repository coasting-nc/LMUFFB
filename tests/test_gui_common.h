#pragma once

#include "../src/gui/GuiLayer.h"
#include <string>

class GuiLayerTestAccess {
public:
    static void DrawMenuBar(LMUFFB::FFBEngine& engine) { LMUFFB::GUI::GuiLayer::DrawMenuBar(engine); }
    static void LaunchLogAnalyzer(const std::string& log_file) { LMUFFB::GUI::GuiLayer::LaunchLogAnalyzer(log_file); }
    static void DrawTuningWindow(LMUFFB::FFBEngine& engine) { LMUFFB::GUI::GuiLayer::DrawTuningWindow(engine); }
    static void DrawDebugWindow(LMUFFB::FFBEngine& engine) { LMUFFB::GUI::GuiLayer::DrawDebugWindow(engine); }
    
#ifdef LMUFFB_UNIT_TEST
    static void GetLastLaunchArgs(std::wstring& wArgs, std::string& cmd);
#endif
};
