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

TEST_CASE(test_gui_id_collision_prevention, "GUI") {
    std::cout << "\nTest: GUI ID Collision Prevention (Issue #70)" << std::endl;

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(800, 600);
    ImGui::NewFrame();

    const char* duplicate_label = "Same Name";
    ImGuiID id1, id2;

    // Simulation of the fix: Using PushID(i)
    {
        ImGui::PushID(0);
        id1 = ImGui::GetID(duplicate_label);
        ImGui::PopID();

        ImGui::PushID(1);
        id2 = ImGui::GetID(duplicate_label);
        ImGui::PopID();

        if (id1 != id2) {
            std::cout << "[PASS] Unique IDs generated for identical labels using PushID" << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Identical IDs generated despite PushID" << std::endl;
            g_tests_failed++;
        }
    }

    // Confirmation of the problem: Without PushID
    {
        ImGuiID id_bad1 = ImGui::GetID(duplicate_label);
        ImGuiID id_bad2 = ImGui::GetID(duplicate_label);

        if (id_bad1 == id_bad2) {
            std::cout << "[INFO] Verified: Identical labels without PushID cause ID collision" << std::endl;
        }
    }

    ImGui::EndFrame();
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
