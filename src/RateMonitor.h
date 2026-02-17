#ifndef RATEMONITOR_H
#define RATEMONITOR_H

#include <chrono>
#include <atomic>

/**
 * @brief Simple utility to monitor event frequency (Hz) over a 1-second sliding window.
 */
class RateMonitor {
public:
    RateMonitor() : m_count(0), m_lastRateScaled(0) {
        m_startTime = std::chrono::steady_clock::now();
    }

    /**
     * @brief Record a single event occurrence.
     */
    void RecordEvent() {
        RecordEventAt(std::chrono::steady_clock::now());
    }

    /**
     * @brief Record an event at a specific time (useful for testing).
     */
    void RecordEventAt(std::chrono::steady_clock::time_point now) {
        m_count++;
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();

        // Update rate every second
        if (duration_ms >= 1000) {
            long count = m_count.exchange(0);
            double rate = (double)count * 1000.0 / (double)duration_ms;
            m_lastRateScaled.store((long)(rate * 100.0));
            m_startTime = now;
        }
    }

    /**
     * @brief Get the last calculated rate in Hz.
     */
    double GetRate() const {
        return (double)m_lastRateScaled.load() / 100.0;
    }

private:
    std::atomic<long> m_count;
    std::chrono::steady_clock::time_point m_startTime;
    std::atomic<long> m_lastRateScaled; // Rate multiplied by 100 for atomic storage
};

#endif // RATEMONITOR_H
