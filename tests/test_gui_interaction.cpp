#include "test_ffb_common.h"
#include "GuiWidgets.h"
#include "imgui.h"

namespace FFBEngineTests {

TEST_CASE(test_gui_decorator_execution, "GUI") {
    std::cout << "\nTest: GUI Decorator Execution" << std::endl;

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable imgui.ini during tests
    io.DisplaySize = ImVec2(1920, 1080); // Set display size to avoid assertion
    
    // Mock a font to avoid assertion in some ImGui versions
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    float val = 0.5f;
    bool decoratorCalled = false;
    ImGui::NewFrame();
    ImGui::Columns(2);
    GuiWidgets::Float("TestDecorator", &val, 0.0f, 1.0f, "%.2f", nullptr, [&](){ decoratorCalled = true; });
    ImGui::EndFrame();
    
    if (decoratorCalled) {
        std::cout << "[PASS] Float Decorator Execution" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Float Decorator NOT executed" << std::endl;
        g_tests_failed++;
    }

    ImGui::DestroyContext(ctx);
}

TEST_CASE(test_gui_result_defaults, "GUI") {
    std::cout << "\nTest: GUI Result Struct Defaults" << std::endl;
    GuiWidgets::Result res;
    if (!res.changed && !res.deactivated) {
        std::cout << "[PASS] Result default values" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Result default values incorrect" << std::endl;
        g_tests_failed++;
    }
}

} // namespace FFBEngineTests
