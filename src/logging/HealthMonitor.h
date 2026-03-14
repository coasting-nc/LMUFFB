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
    bool physics_low = false; // New v0.7.117 (Issue #217)

    double loop_rate = 0.0;
    double telem_rate = 0.0;
    double torque_rate = 0.0;
    double physics_rate = 0.0; // New v0.7.117 (Issue #217)
    double expected_torque_rate = 0.0;

    // Session and Player State (#269, #274)
    bool is_connected = false;
    bool session_active = false;
    long session_type = -1;
    bool is_realtime = false;
    signed char player_control = -2;
};

class HealthMonitor {
public:
    /**
     * @brief Checks if rates are within acceptable ranges.
     * @param loop Current FFB loop rate (Hz).
     * @param telem Current telemetry update rate (Hz).
     * @param torque Current torque update rate (Hz).
     * @param torqueSource Active torque source (0=Legacy, 1=Direct).
     * @param physics Current physics update rate (Hz).
     * @param isConnected True if connected to LMU Shared Memory.
     * @param sessionActive True if a session is loaded.
     * @param sessionType Current session type (0=Test Day, 1=Practice, 5=Qualifying, 10=Race).
     * @param isRealtime True if in realtime (driving).
     * @param playerControl Current player control state (0=Player, 1=AI, etc).
     */
    static HealthStatus Check(double loop, double telem, double torque, int torqueSource, double physics = 0.0,
                              bool isConnected = false, bool sessionActive = false, long sessionType = -1, bool isRealtime = false, signed char playerControl = -2) {
        HealthStatus status;
        status.loop_rate = loop;
        status.telem_rate = telem;
        status.torque_rate = torque;
        status.physics_rate = physics;
        status.expected_torque_rate = (torqueSource == 1) ? 400.0 : 100.0;
        
        status.is_connected = isConnected;
        status.session_active = sessionActive;
        status.session_type = sessionType;
        status.is_realtime = isRealtime;
        status.player_control = playerControl;

        // Loop: Target 1000Hz (USB). Warn below 950Hz.
        if (loop > 1.0 && loop < 950.0) {
            status.loop_low = true;
            status.is_healthy = false;
        }

        // Physics: Target 400Hz. Warn below 380Hz.
        if (physics > 1.0 && physics < 380.0) {
            status.physics_low = true;
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
