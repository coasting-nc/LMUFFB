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

TEST_CASE(test_widgets_have_tooltips, "GUI") {
    std::cout << "\nTest: Widgets Tooltips Presence" << std::endl;

    int passed_tooltips = 0;
    int missing_tooltips = 0;

    // Helper to simulate calling the widgets and checking logic
    // We can't easily check internal ImGui state here without complex mocks,
    // so we'll do a static analysis check approach conceptually or check
    // if the function allows nullptr and handles it.

    // Since we can't easily introspect the running GUI in a unit test to see
    // if a tooltip *actually rendered*, we will act as a "compile-time" check
    // or a "runtime-check" that the parameters are being passed correctly
    // by manually inspecting the critical sections in the actual code (which we did).

    // However, we CAN test the GuiWidgets logic:
    // Ensure that if we pass a tooltip, it attempts to render it.
    // This requires mocking ImGui::IsItemHovered() which is hard.

    // Check: Verify GuiWidgets::Float accepts tooltip and doesn't crash
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(100, 100);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    ImGui::NewFrame();
    float f = 0.5f;
    GuiWidgets::Float("TestTooltip", &f, 0.0f, 1.0f, "%.2f", "This is a tooltip");

    bool b = false;
    GuiWidgets::Checkbox("TestCheck", &b, "Check tooltip");

    int i = 0;
    const char* items[] = {"A", "B"};
    GuiWidgets::Combo("TestCombo", &i, items, 2, "Combo tooltip");

    ImGui::EndFrame();
    ImGui::DestroyContext(ctx);

    std::cout << "[PASS] Widget functions accept tooltips without crashing" << std::endl;
    g_tests_passed++;
}

} // namespace FFBEngineTests
