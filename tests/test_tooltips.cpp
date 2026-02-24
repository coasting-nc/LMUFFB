#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "../src/Tooltips.h"
#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_tooltip_line_lengths, "UI") {
    std::cout << "\nTest: ToolTip Line Lengths (Issue #179)" << std::endl;

    const size_t MAX_LINE_LENGTH = 80;
    int fail_count = 0;

    for (const char* tooltip : Tooltips::ALL) {
        if (!tooltip) continue;

        std::string s(tooltip);
        std::stringstream ss(s);
        std::string line;

        while (std::getline(ss, line, '\n')) {
            if (line.length() > MAX_LINE_LENGTH) {
                std::cout << "  [FAIL] Line too long (" << line.length() << " chars) in tooltip starting with: "
                          << s.substr(0, 20) << "..." << std::endl;
                std::cout << "         Line: " << line << std::endl;
                fail_count++;
            }
        }
    }

    if (fail_count > 0) {
        std::cout << "  Total failures: " << fail_count << std::endl;
    }
    ASSERT_TRUE(fail_count == 0);
}

} // namespace FFBEngineTests
