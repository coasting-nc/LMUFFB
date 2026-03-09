#include "test_ffb_common.h"
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

TEST_CASE(test_analyzer_bundling_integrity, "Distribution") {
    std::cout << "Running test_analyzer_bundling_integrity..." << std::endl;
#if defined(LMU_SOURCE_DIR) && defined(LMU_BINARY_DIR)
    fs::path source_root = LMU_SOURCE_DIR;
    fs::path binary_root = LMU_BINARY_DIR;

    // The tools are in binary_root/Release/tools on Windows (Multi-config)
    // or binary_root/tools on Linux (Single-config)
    
    fs::path tools_build_path = binary_root / "tools" / "lmuffb_log_analyzer";
    if (!fs::exists(tools_build_path)) {
       // Check for MSVC "Release" or "Debug" subfolders
       if (fs::exists(binary_root / "Release" / "tools" / "lmuffb_log_analyzer")) {
           tools_build_path = binary_root / "Release" / "tools" / "lmuffb_log_analyzer";
       } else if (fs::exists(binary_root / "Debug" / "tools" / "lmuffb_log_analyzer")) {
           tools_build_path = binary_root / "Debug" / "tools" / "lmuffb_log_analyzer";
       }
    }

    if (!fs::exists(tools_build_path)) {
        std::string msg = "Could not find tools build path in: " + tools_build_path.string();
        FAIL_TEST(msg.c_str());
        return;
    }
    
    fs::path analyzers_build_path = tools_build_path / "analyzers";
    ASSERT_TRUE(fs::exists(analyzers_build_path));
    
    // --- Regression Test for Issue #321 ---
    // Specifically verify lateral_analyzer.py is present
    ASSERT_TRUE(fs::exists(analyzers_build_path / "lateral_analyzer.py"));

    fs::path tools_src_path = source_root / "tools" / "lmuffb_log_analyzer";
    fs::path analyzers_src_path = tools_src_path / "analyzers";

    ASSERT_TRUE(fs::exists(analyzers_src_path));

    // For every .py file in source analyzers, it MUST exist in build analyzers
    int py_count = 0;
    for (const auto& entry : fs::directory_iterator(analyzers_src_path)) {
        if (entry.path().extension() == ".py") {
            fs::path target = analyzers_build_path / entry.path().filename();
            if (!fs::exists(target)) {
                std::string msg = "Missing analyzer in build: " + entry.path().filename().string();
                FAIL_TEST(msg.c_str());
            } else {
                FFBEngineTests::g_tests_passed++;
            }
            py_count++;
        }
    }
    ASSERT_GT(py_count, 0);

    // Also check root tool files (.py, .txt, .md)
    int root_count = 0;
    for (const auto& entry : fs::directory_iterator(tools_src_path)) {
        if (entry.is_regular_file()) {
           std::string ext = entry.path().extension().string();
           if (ext == ".py" || ext == ".txt" || ext == ".md") {
               fs::path target = tools_build_path / entry.path().filename();
               if (!fs::exists(target)) {
                   std::string msg = "Missing root tool file in build: " + entry.path().filename().string();
                   FAIL_TEST(msg.c_str());
               } else {
                   FFBEngineTests::g_tests_passed++;
               }
               root_count++;
           }
        }
    }
    ASSERT_GT(root_count, 0);

#else
    FAIL_TEST("LMU_SOURCE_DIR or LMU_BINARY_DIR not defined. Cannot run distribution test.");
#endif
}
