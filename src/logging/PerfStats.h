#ifndef PERF_STATS_H
#define PERF_STATS_H

#include <cmath>
#include <limits>

// Stats helper
struct ChannelStats {
    // Session-wide stats (Persistent)
    double session_min = 1e9;
    double session_max = -1e9;
    
    // Interval stats (Reset every second)
    double interval_sum = 0.0;
    long interval_count = 0;
    
    // Latched values for display/consumption by other threads (Interval)
    double l_avg = 0.0;
    // Latched values for display/consumption by other threads (Session)
    double l_min = 0.0;
    double l_max = 0.0;
    
    void Update(double val) {
        // Update Session Min/Max
        if (val < session_min) session_min = val;
        if (val > session_max) session_max = val;
        
        // Update Interval Accumulator
        interval_sum += val;
        interval_count++;
    }
    
    // Called every interval (e.g. 1s) to latch data and reset interval counters
    void ResetInterval() {
        if (interval_count > 0) {
            l_avg = interval_sum / interval_count;
        } else {
            l_avg = 0.0;
        }
        // Latch current session min/max for display
        l_min = session_min;
        l_max = session_max;
        
        // Reset interval data
        interval_sum = 0.0; 
        interval_count = 0;
    }
    
    // Compatibility helper
    double Avg() { return interval_count > 0 ? interval_sum / interval_count : 0.0; }
    void Reset() { ResetInterval(); }
};

#endif // PERF_STATS_H
