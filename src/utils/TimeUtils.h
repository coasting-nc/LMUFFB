#ifndef TIMEOUTILS_H
#define TIMEOUTILS_H

#include <chrono>

namespace LMUFFB {
namespace Utils {

#ifdef LMUFFB_UNIT_TEST
// These are defined in main_test_runner.cpp
extern std::chrono::steady_clock::time_point g_mock_time;
extern bool g_use_mock_time;
#endif

namespace TimeUtils {
    /**
     * @brief A simple interface for providing the current time.
     * Allows for mocking in unit tests.
     */
    inline std::chrono::steady_clock::time_point GetTime() {
#ifdef LMUFFB_UNIT_TEST
        if (g_use_mock_time) return g_mock_time;
#endif
        return std::chrono::steady_clock::now();
    }
} // namespace TimeUtils
} // namespace Utils

// Temporary bridge for legacy code
namespace TimeUtils = Utils::TimeUtils;
#ifdef LMUFFB_UNIT_TEST
using Utils::g_mock_time;
using Utils::g_use_mock_time;
#endif

} // namespace LMUFFB

#endif // TIMEOUTILS_H
