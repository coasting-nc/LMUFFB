#include "test_ffb_common.h"
#include "imgui.h"
#include "../src/gui/GuiLayer.h"

class GuiLayerTestAccess {
public:
    static void DrawMenuBar(FFBEngine& engine) { GuiLayer::DrawMenuBar(engine); }
};

namespace FFBEngineTests {

TEST_CASE(test_gui_menu_bar_presence, "GUI") {
    std::cout << "\nTest: GUI Menu Bar Presence" << std::endl;

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    FFBEngine engine;
    
    ImGui::NewFrame();
    // We haven't implemented DrawMenuBar yet, so this will fail to compile if I add it now.
    // I will first implement the declaration in GuiLayer.h.
    GuiLayerTestAccess::DrawMenuBar(engine); 
    ImGui::EndFrame();

    // In a unit test, it's hard to verify if the menu bar actually rendered without mocking ImGui internals.
    // But we can check if it at least runs without crashing and covers the code.
    
    ImGui::DestroyContext(ctx);
    std::cout << "[PASS] GUI Menu Bar render finished" << std::endl;
    g_tests_passed++;
}

} // namespace FFBEngineTests
