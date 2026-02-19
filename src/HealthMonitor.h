#ifndef HEALTHMONITOR_H
#define HEALTHMONITOR_H

/**
 * @brief Logic for determining if system sample rates are healthy.
 * Issue #133: Adjusted thresholds to be source-aware.
 */
struct HealthStatus {
    bool is_healthy = true;
    bool loop_low = false;
    bool telem_low = false;
    bool torque_low = false;

    double loop_rate = 0.0;
    double telem_rate = 0.0;
    double torque_rate = 0.0;
    double expected_torque_rate = 0.0;
};

class HealthMonitor {
public:
    /**
     * @brief Checks if rates are within acceptable ranges.
     * @param loop Current FFB loop rate (Hz).
     * @param telem Current telemetry update rate (Hz).
     * @param torque Current torque update rate (Hz).
     * @param torqueSource Active torque source (0=Legacy, 1=Direct).
     */
    static HealthStatus Check(double loop, double telem, double torque, int torqueSource) {
        HealthStatus status;
        status.loop_rate = loop;
        status.telem_rate = telem;
        status.torque_rate = torque;
        status.expected_torque_rate = (torqueSource == 1) ? 400.0 : 100.0;

        // Loop: Target 400Hz. Warn below 360Hz.
        if (loop > 1.0 && loop < 360.0) {
            status.loop_low = true;
            status.is_healthy = false;
        }

        // Telemetry (Standard LMU): Target 100Hz. Warn below 90Hz.
        if (telem > 1.0 && telem < 90.0) {
            status.telem_low = true;
            status.is_healthy = false;
        }

        // Torque: Target depends on source.
        if (torque > 1.0 && torque < (status.expected_torque_rate * 0.9)) {
            status.torque_low = true;
            status.is_healthy = false;
        }

        return status;
    }
};

#endif // HEALTHMONITOR_H
