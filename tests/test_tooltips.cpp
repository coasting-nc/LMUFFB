#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "../src/gui/Tooltips.h"
#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_tooltip_line_lengths, "UI") {
    std::cout << "\nTest: ToolTip Line Lengths (Issue #179)" << std::endl;

    const size_t MAX_LINE_LENGTH = 80;
    int fail_count = 0;

    // Load Tooltips.h to provide better diagnostics
    std::vector<std::string> header_lines;
    std::string header_path = "src/gui/Tooltips.h";
    std::ifstream hf(header_path);
    if (!hf.is_open()) {
        hf.close();
        hf.open("../" + header_path);
    }
    if (hf.is_open()) {
        std::string h_line;
        while (std::getline(hf, h_line)) header_lines.push_back(h_line);
        hf.close();
    }

    for (const char* tooltip : LMUFFB::Tooltips::ALL) {
        if (!tooltip) continue;

        std::string s(tooltip);
        std::stringstream ss(s);
        std::string line;
        int line_index = 0;

        while (std::getline(ss, line, '\n')) {
            line_index++;
            if (line.length() > MAX_LINE_LENGTH) {
                // Find where this tooltip is in the header to get variable name and line number
                int found_line_num = -1;
                std::string var_name = "Unknown";
                
                if (!header_lines.empty()) {
                    // Search for the first part of the tooltip in the header file
                    size_t first_nl = s.find('\n');
                    std::string search_snippet = (first_nl == std::string::npos) ? s : s.substr(0, first_nl);
                    if (search_snippet.length() > 40) search_snippet = search_snippet.substr(0, 40);

                    for (size_t i = 0; i < header_lines.size(); ++i) {
                        if (header_lines[i].find(search_snippet) != std::string::npos) {
                            found_line_num = (int)i + 1;
                            
                            // Extract variable name from the line (e.g., inline constexpr const char* NAME = ...)
                            size_t asterisk_pos = header_lines[i].find("*");
                            size_t equal_pos = header_lines[i].find("=");
                            if (asterisk_pos != std::string::npos && equal_pos != std::string::npos && equal_pos > asterisk_pos) {
                                size_t start = asterisk_pos + 1;
                                while (start < equal_pos && std::isspace(header_lines[i][start])) start++;
                                size_t end = equal_pos - 1;
                                while (end > start && std::isspace(header_lines[i][end])) end--;
                                if (start <= end) var_name = header_lines[i].substr(start, end - start + 1);
                            }
                            break;
                        }
                    }
                }

                std::stringstream os;
                os << "Line too long (" << line.length() << " > " << MAX_LINE_LENGTH << ") in "
                   << "tooltip \"" << var_name << "\" (line " << line_index << " of the string content)";
                if (found_line_num != -1) {
                    os << " at Tooltips.h:" << found_line_num;
                }
                os << ".\n         Line content: \"" << line << "\"";
                
                FAIL_TEST(os.str());
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
