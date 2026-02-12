#include "test_ffb_common.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>

namespace FFBEngineTests {

TEST_CASE(test_gui_tooltips_presence_static, "GUI") {
    std::cout << "\nTest: GUI Tooltips Presence (Static Analysis)" << std::endl;

    // Try multiple paths to find the source file relative to the test runner
    std::vector<std::string> paths = {
        "src/GuiLayer_Common.cpp",
        "../src/GuiLayer_Common.cpp",
        "../../src/GuiLayer_Common.cpp",
        "../../../src/GuiLayer_Common.cpp",
        "../../../../src/GuiLayer_Common.cpp"
    };

    std::string found_path = "";
    for (const auto& p : paths) {
        if (std::filesystem::exists(p)) {
            found_path = p;
            break;
        }
    }

    if (found_path.empty()) {
        std::cout << "[WARN] Could not find src/GuiLayer_Common.cpp for static analysis. Skipping." << std::endl;
        g_tests_passed++;
        return;
    }

    std::cout << "  Analyzing: " << found_path << std::endl;

    std::ifstream file(found_path);
    if (!file.is_open()) {
        std::cout << "[FAIL] Failed to open " << found_path << std::endl;
        g_tests_failed++;
        return;
    }

    std::string line;
    std::vector<std::string> missing;

    // Using custom delimiter "regex" to avoid collision with )" in the regex pattern
    // We look for calls where the tooltip parameter is nullptr, NULL, or empty ""
    std::regex float_reg(R"regex(FloatSetting\s*\(\s*"([^"]+)"\s*,[^,]+,[^,]+,[^,]+,[^,]+,\s*(nullptr|NULL|""|"")\s*[,|\)])regex");
    std::regex bool_reg(R"regex(BoolSetting\s*\(\s*"([^"]+)"\s*,[^,]+,\s*(nullptr|NULL|""|"")\s*[,|\)])regex");
    std::regex int_reg(R"regex(IntSetting\s*\(\s*"([^"]+)"\s*,[^,]+,[^,]+,[^,]+,\s*(nullptr|NULL|""|"")\s*[,|\)])regex");

    std::regex gw_float_reg(R"regex(GuiWidgets::Float\s*\(\s*"([^"]+)"\s*,[^,]+,[^,]+,[^,]+,[^,]+,\s*(nullptr|NULL|""|"")\s*[,|\)])regex");
    std::regex gw_bool_reg(R"regex(GuiWidgets::Checkbox\s*\(\s*"([^"]+)"\s*,[^,]+,\s*(nullptr|NULL|""|"")\s*[,|\)])regex");
    std::regex gw_combo_reg(R"regex(GuiWidgets::Combo\s*\(\s*"([^"]+)"\s*,[^,]+,[^,]+,[^,]+,\s*(nullptr|NULL|""|"")\s*[,|\)])regex");

    int line_num = 0;
    while (std::getline(file, line)) {
        line_num++;
        std::smatch match;

        bool fail = false;
        std::string widget_name = "";
        std::string helper_type = "";

        if (std::regex_search(line, match, float_reg)) { helper_type = "FloatSetting"; widget_name = match[1].str(); fail = true; }
        else if (std::regex_search(line, match, bool_reg)) { helper_type = "BoolSetting"; widget_name = match[1].str(); fail = true; }
        else if (std::regex_search(line, match, int_reg)) { helper_type = "IntSetting"; widget_name = match[1].str(); fail = true; }
        else if (std::regex_search(line, match, gw_float_reg)) { helper_type = "GuiWidgets::Float"; widget_name = match[1].str(); fail = true; }
        else if (std::regex_search(line, match, gw_bool_reg)) { helper_type = "GuiWidgets::Checkbox"; widget_name = match[1].str(); fail = true; }
        else if (std::regex_search(line, match, gw_combo_reg)) { helper_type = "GuiWidgets::Combo"; widget_name = match[1].str(); fail = true; }

        if (fail) {
            missing.push_back("Line " + std::to_string(line_num) + ": " + helper_type + " \"" + widget_name + "\"");
        }
    }

    if (missing.empty()) {
        std::cout << "[PASS] All identified widgets have non-empty tooltips." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Missing tooltips found in source code:" << std::endl;
        for (const auto& m : missing) {
            std::cout << "  - " << m << std::endl;
        }
        g_tests_failed++;
    }
}

} // namespace FFBEngineTests
