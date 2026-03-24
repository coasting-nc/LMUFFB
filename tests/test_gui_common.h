#pragma once

#include "../src/gui/GuiLayer.h"
#include <string>

class GuiLayerTestAccess {
public:
    static void DrawMenuBar(FFBEngine& engine) { GuiLayer::DrawMenuBar(engine); }
    static void LaunchLogAnalyzer(const std::string& log_file) { GuiLayer::LaunchLogAnalyzer(log_file); }
    
#ifdef LMUFFB_UNIT_TEST
    static void GetLastLaunchArgs(std::wstring& wArgs, std::string& cmd);
#endif
};
