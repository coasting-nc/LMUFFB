#include "test_ffb_common.h"
#include "PerfStats.h"

namespace FFBEngineTests {

TEST_CASE(test_channel_stats_average, "Performance") {
    ChannelStats stats;
    stats.Update(10.0);
    stats.Update(20.0);
    stats.Update(30.0);
    
    ASSERT_NEAR(stats.Avg(), 20.0, 0.001);
}

TEST_CASE(test_channel_stats_resets, "Performance") {
    ChannelStats stats;
    
    // Interval 1
    stats.Update(10.0);
    stats.Update(100.0);
    ASSERT_NEAR(stats.session_max, 100.0, 0.001);
    
    stats.ResetInterval();
    ASSERT_NEAR(stats.interval_sum, 0.0, 0.001);
    ASSERT_NEAR(stats.l_max, 100.0, 0.001); // Latched session max
    
    // Interval 2
    stats.Update(20.0);
    stats.Update(50.0);
    
    ASSERT_NEAR(stats.interval_sum, 70.0, 0.001);
    ASSERT_NEAR(stats.session_max, 100.0, 0.001); // Still 100 from Interval 1
    
    stats.Update(150.0);
    ASSERT_NEAR(stats.session_max, 150.0, 0.001);
}

TEST_CASE(test_channel_stats_empty_reset, "Performance") {
    ChannelStats stats;
    stats.ResetInterval();
    ASSERT_NEAR(stats.l_avg, 0.0, 0.001);
}

} // namespace FFBEngineTests
