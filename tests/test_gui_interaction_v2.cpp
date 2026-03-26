#include "test_ffb_common.h"
#include "test_gui_common.h"
#include "GuiWidgets.h"
#include "imgui.h"
#include "../src/gui/GuiLayer.h"
#include "../src/core/Config.h"

static const float CONFIG_PANEL_WIDTH = 500.0f;
static const int PLOT_BUFFER_SIZE = 4000;

namespace FFBEngineTests {

TEST_CASE(test_gui_interaction_v2, "GUI") {
    std::cout << "\nTest: GUI Interaction V2" << std::endl;

    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    FFBEngine engine;

    // 1. Test DisplayRate color branches
    {
        ImGui::NewFrame();

        // Green
        engine.m_ffb_rate = 400.0;
        GuiLayerTestAccess::DrawDebugWindow(engine);

        // Yellow
        engine.m_ffb_rate = 300.0;
        GuiLayerTestAccess::DrawDebugWindow(engine);

        // Red
        engine.m_ffb_rate = 100.0;
        GuiLayerTestAccess::DrawDebugWindow(engine);

        ImGui::EndFrame();
    }

    // 2. Test Config::show_graphs toggle
    {
        Config::show_graphs = false;
        ImGui::NewFrame();
        GuiLayerTestAccess::DrawTuningWindow(engine);
        GuiLayerTestAccess::DrawDebugWindow(engine);
        ImGui::EndFrame();

        Config::show_graphs = true;
        ImGui::NewFrame();
        GuiLayerTestAccess::DrawTuningWindow(engine);
        GuiLayerTestAccess::DrawDebugWindow(engine);
        ImGui::EndFrame();
    }

    // 3. Diverse UI states
    {
        engine.m_advanced.soft_lock_enabled = true;
        engine.m_braking.abs_pulse_enabled = true;
        engine.m_braking.lockup_enabled = true;
        engine.m_vibration.spin_enabled = true;
        engine.m_vibration.slide_enabled = true;
        engine.m_vibration.road_enabled = true;
        engine.m_front_axle.flatspot_suppression = true;
        engine.m_front_axle.static_notch_enabled = true;
        engine.m_slope_detection.enabled = true;

        ImGui::NewFrame();
        GuiLayerTestAccess::DrawTuningWindow(engine);
        ImGui::EndFrame();
    }


    // 5. Systematic Fuzzing of Tuning Window
    {
        Config::show_graphs = false;
        for (int y = 0; y < 2000; y += 10) {
            ImGui::NewFrame();
            io.MousePos = ImVec2(100, (float)y);
            io.MouseDown[0] = (y % 20 == 0);
            GuiLayerTestAccess::DrawTuningWindow(engine);
            ImGui::EndFrame();
        }
        std::cout << "[PASS] Tuning Window systematic fuzzing" << std::endl;
        g_tests_passed++;
    }

    // 6. Systematic Fuzzing of Debug Window
    {
        Config::show_graphs = true;
        FFBSnapshot s;
        memset(&s, 0, sizeof(s));
        s.total_output = 0.5f;
        s.warn_dt = true;
        FFBEngineTestAccess::AddSnapshot(engine, s);

        for (int y = 0; y < 1000; y += 20) {
            ImGui::NewFrame();
            io.MousePos = ImVec2(CONFIG_PANEL_WIDTH + 100, (float)y);
            io.MouseDown[0] = true;
            GuiLayerTestAccess::DrawDebugWindow(engine);
            ImGui::EndFrame();
        }
        std::cout << "[PASS] Debug Window systematic fuzzing" << std::endl;
        g_tests_passed++;
    }

    ImGui::DestroyContext(ctx);
    std::cout << "[PASS] GUI Interaction V2 finished" << std::endl;
}

} // namespace FFBEngineTests
