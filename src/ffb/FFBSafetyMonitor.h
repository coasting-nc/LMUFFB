#ifndef FFBSAFETYMONITOR_H
#define FFBSAFETYMONITOR_H

#include <cmath>
#include <algorithm>
#include <cstring>
#include "io/lmu_sm_interface/InternalsPluginWrapper.h"
#include "logging/Logger.h"
#include "utils/StringUtils.h"

class FFBSafetyMonitor {
public:
    static constexpr float SAFETY_SLEW_NORMAL = 1000.0f;
    static constexpr float SAFETY_SLEW_RESTRICTED = 100.0f;

    FFBSafetyMonitor() = default;

    // FFB Safety Settings
    float m_safety_window_duration = 0.0f;
    float m_safety_gain_reduction = 0.3f;
    float m_safety_smoothing_tau = 0.2f;
    float m_spike_detection_threshold = 500.0f;
    float m_immediate_spike_threshold = 1500.0f;
    float m_safety_slew_full_scale_time_s = 1.0f; 
    bool  m_stutter_safety_enabled = false;
    float m_stutter_threshold = 1.5f;

    // API methods for FFBEngine
    double GetSafetyTimer() const { return safety_timer; }
    void ClearSafetySmoothedForce() { safety_smoothed_force = 0.0; }
    void SetSafetySmoothedForce(double f) { safety_smoothed_force = f; }
    void SetTimePtr(const double* ptr) { m_time_ptr = ptr; }

    bool GetLastAllowed() const { return last_allowed; }
    void SetLastAllowed(bool val) { last_allowed = val; }
    signed char GetLastControl() const { return last_mControl; }
    void SetLastControl(signed char val) { last_mControl = val; }

    bool IsFFBAllowed(const VehicleScoringInfoV01& scoring, unsigned char gamePhase) const;
    
    void TriggerSafetyWindow(const char* reason, double now = -1.0);
    double ApplySafetySlew(double target_force, double current_output_force, double dt, bool restricted, double now = -1.0);
    
    double ProcessSafetyMitigation(double norm_force, double dt);
    void UpdateTockDetection(double steering, double norm_force, double dt);

    // Legacy 3-arg version for tests (stateful)
    // Primarily for standalone testing where a real time source is not available.
    [[nodiscard]] double ApplySafetySlew(double target_force, double dt, bool restricted);

    // Internal state (Kept public for legacy test compatibility as per Approach A)
    signed char last_mControl = -2;
    double safety_timer = 0.0;
    double safety_smoothed_force = 0.0;
    bool safety_is_seeded = false;
    int spike_counter = 0;
    double tock_timer = 0.0;
    double last_tock_log_time = -999.0;
    double last_reset_log_time = -999.0;
    char last_reset_reason[64] = "";
    double last_massive_spike_log_time = -999.0;
    double last_high_spike_log_time = -999.0;
    bool last_allowed = true;
    double m_last_now = 0.0; 
    const double* m_time_ptr = nullptr; 

    // Soft Lock State
    bool was_soft_locked = false;
    bool soft_lock_significant = false;
};

#endif // FFBSAFETYMONITOR_H
