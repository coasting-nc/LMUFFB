#include "test_ffb_common.h"
#include "test_gui_common.h"
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace FFBEngineTests {

TEST_CASE(test_gui_launch_log_analyzer_windows, "GUI") {
    std::cout << "\nTest: GUI Launch Log Analyzer (Windows Simulation)" << std::endl;

#ifdef _WIN32
    std::string test_log = "c:\\path\\to\\my_log.txt";
    GuiLayerTestAccess::LaunchLogAnalyzer(test_log);

    std::wstring wArgs;
    std::string cmd;
#ifdef LMUFFB_UNIT_TEST
    GuiLayerTestAccess::GetLastLaunchArgs(wArgs, cmd);
    
    std::wstring expected = L"/k python -m lmuffb_log_analyzer.cli analyze-full \"c:\\path\\to\\my_log.txt\"";
    
    ASSERT_EQ_WSTR(wArgs.c_str(), expected.c_str());
#else
    std::cout << "[SKIP] LMUFFB_UNIT_TEST not defined, cannot verify args" << std::endl;
#endif
#else
    std::cout << "[SKIP] Not on Windows" << std::endl;
#endif
    g_tests_passed++;
}

TEST_CASE(test_gui_launch_log_analyzer_linux, "GUI") {
    std::cout << "\nTest: GUI Launch Log Analyzer (Linux Simulation)" << std::endl;

#ifndef _WIN32
    std::string test_log = "/path/to/my_log.txt";
    GuiLayerTestAccess::LaunchLogAnalyzer(test_log);

    std::wstring wArgs;
    std::string cmd;
#ifdef LMUFFB_UNIT_TEST
    GuiLayerTestAccess::GetLastLaunchArgs(wArgs, cmd);
    
    // On Linux, the cmd is something like:
    // "PYTHONPATH=" + python_path + " python3 -m lmuffb_log_analyzer.cli analyze-full \"/path/to/my_log.txt\""
    // Since we don't know the exact python_path (it depends on current dir), we just check for the key parts.
    
    ASSERT_NE(cmd.find("python3 -m lmuffb_log_analyzer.cli analyze-full \"/path/to/my_log.txt\""), std::string::npos);
    ASSERT_NE(cmd.find("PYTHONPATH="), std::string::npos);
#else
    std::cout << "[SKIP] LMUFFB_UNIT_TEST not defined, cannot verify cmd" << std::endl;
#endif
#else
    std::cout << "[SKIP] Not on Linux" << std::endl;
#endif
    g_tests_passed++;
}

} // namespace FFBEngineTests
