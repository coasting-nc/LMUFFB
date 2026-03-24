#include "test_ffb_common.h"
#include "test_gui_common.h"
#include "imgui.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

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
    GuiLayerTestAccess::DrawMenuBar(engine); 
    ImGui::EndFrame();

    ImGui::DestroyContext(ctx);
    std::cout << "[PASS] GUI Menu Bar render finished" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_gui_menu_bar_with_log, "GUI") {
    std::cout << "\nTest: GUI Menu Bar With Log Search" << std::endl;

    TestDirectoryGuard log_guard("logs");
    {
        std::ofstream log_file("logs/test_log_2026_03_24_12_00_00.txt");
        log_file << "test log content";
    }

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    FFBEngine engine;
    
    ImGui::NewFrame();
    // Simulate menu item interaction
    bool menu_open = ImGui::BeginMainMenuBar();
    if (menu_open) {
        if (ImGui::BeginMenu("Tools")) {
            // This is where DrawMenuBar logic runs inside our class
            GuiLayerTestAccess::DrawMenuBar(engine);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::EndFrame();

    ImGui::DestroyContext(ctx);
    std::cout << "[PASS] GUI Menu Bar with log search finished" << std::endl;
    g_tests_passed++;
}

} // namespace FFBEngineTests
