#include "FFBSafetyMonitor.h"

bool FFBSafetyMonitor::IsFFBAllowed(const VehicleScoringInfoV01& scoring, unsigned char gamePhase) const {
    // 1. Mute if not player vehicle
    if (!scoring.mIsPlayer) return false;

    // 2. Mute if not in player control (AI or Garage)
    if (scoring.mControl != 0) return false; 

    // 3. Mute if in Garage Stall
    if (scoring.mInGarageStall) return false;

    // 4. Mute if Disqualified
    if (scoring.mFinishStatus == 3) return false;

    return true;
}

void FFBSafetyMonitor::TriggerSafetyWindow(const char* reason, double now) {
    if (now >= 0.0) {
        m_last_now = now;
    } else if (m_time_ptr) {
        m_last_now = *m_time_ptr;
    }
    double current_now = m_last_now;

    bool different_reason = std::strcmp(reason, last_reset_reason) != 0;
    bool timer_expired = safety_timer <= 0.0;

    if (timer_expired || different_reason) {
        // Always log if the window was inactive or the reason changed
        Logger::Get().LogFile("[Safety] Triggered Safety Window (%.1fs). Reason: %s", 
            (double)m_safety_window_duration, reason);
        last_reset_log_time = current_now;
        StringUtils::SafeCopy(last_reset_reason, 64, reason);
    } else {
        // Same reason, apply 1-second throttling to avoid spam
        if (current_now > (last_reset_log_time + 1.0)) {
            Logger::Get().LogFile("[Safety] Triggered Safety Window (%.1fs). Reason: %s (Subsequent)", 
                (double)m_safety_window_duration, reason);
            last_reset_log_time = current_now;
        }
    }

    safety_timer = (double)m_safety_window_duration;
    safety_is_seeded = false;
    spike_counter = 0; 
}

double FFBSafetyMonitor::ApplySafetySlew(double target_force, double current_output_force, double dt, bool restricted, double now) {
    if (now >= 0.0) {
        m_last_now = now;
    } else if (m_time_ptr) {
        m_last_now = *m_time_ptr;
    }
    double current_now = m_last_now;

    // Check for non-finite inputs
    if (!std::isfinite(target_force)) return 0.0;
    if (!std::isfinite(current_output_force)) return 0.0;

    // 1. Determine slew limit
    double slew_limit = restricted ? (double)SAFETY_SLEW_RESTRICTED : (double)SAFETY_SLEW_NORMAL;
    
    // If safety window is active, we apply the user-defined slew limit (Issue #316)
    if (safety_timer > 0.0 && m_safety_slew_full_scale_time_s > 0.01f) {
        slew_limit = 1.0 / (double)m_safety_slew_full_scale_time_s;
    }

    double max_delta = slew_limit * dt;
    double delta = target_force - current_output_force;
    double requested_rate = std::abs(delta) / (dt + 1e-9);

    // 2. Spike Detection (independent of slew limit)
    if (requested_rate > (double)m_immediate_spike_threshold) {
         if (current_now > (last_massive_spike_log_time + 5.0)) {
            Logger::Get().LogFile("[Safety] Massive Spike! Rate: %.0f u/s (Limit: %.0f). Triggering Safety.", 
                requested_rate, (double)m_immediate_spike_threshold);
            last_massive_spike_log_time = current_now;
        }
        TriggerSafetyWindow("Massive Spike", current_now);
    } else if (requested_rate > (double)m_spike_detection_threshold) {
        spike_counter++;
        if (spike_counter >= 5) { // 5 consecutive frames of high slew
            if (current_now > (last_high_spike_log_time + 5.0)) {
                Logger::Get().LogFile("[Safety] Sustained High Slew (%.0f u/s). Triggering Safety.", requested_rate);
                last_high_spike_log_time = current_now;
            }
            TriggerSafetyWindow("High Spike", current_now);
        }
    } else {
        spike_counter = (std::max)(0, spike_counter - 1);
    }

    // 3. Apply Slew
    if (std::abs(delta) > max_delta) {
        delta = std::clamp(delta, -max_delta, max_delta);
    }

    return current_output_force + delta;
}

double FFBSafetyMonitor::ProcessSafetyMitigation(double norm_force, double dt) {
    if (safety_timer > 0.0) {
        // Apply extra gain reduction
        norm_force *= (double)m_safety_gain_reduction;

        // Apply extra smoothing to the final output to blunt any jitter
        // Using a 200ms EMA (Issue #314)
        // On first frame of safety window, seed the smoothed force
        if (!safety_is_seeded) {
            safety_smoothed_force = norm_force;
            safety_is_seeded = true;
        } else {
            double safety_alpha = dt / ((double)m_safety_smoothing_tau + dt);
            safety_smoothed_force += safety_alpha * (norm_force - safety_smoothed_force);
        }
        norm_force = safety_smoothed_force;

        safety_timer -= dt;
        if (safety_timer <= 0.0) {
            safety_timer = 0.0;
            Logger::Get().LogFile("[Safety] Exited Safety Mode");
        }
    }
    return norm_force;
}

void FFBSafetyMonitor::UpdateTockDetection(double steering, double norm_force, double dt) {
    if (std::abs(steering) > 0.95 && std::abs(norm_force) > 0.8) {
        tock_timer += dt;
        if (tock_timer > 1.0) { // Pinned for 1 second
            double current_now = (m_time_ptr) ? *m_time_ptr : m_last_now;
            if (current_now > (last_tock_log_time + 5.0)) {
                Logger::Get().LogFile("[Safety] Full Tock Detected: Force %.2f at %.1f%% lock",
                    norm_force, steering * 100.0);
                last_tock_log_time = current_now;
            }
            tock_timer = 0.0;
        }
    } else {
        tock_timer = (std::max)(0.0, tock_timer - dt);
    }
}

double FFBSafetyMonitor::ApplySafetySlew(double target_force, double dt, bool restricted) {
    double now = (m_time_ptr) ? *m_time_ptr : m_last_now;
    if (!m_time_ptr) m_last_now += dt; 
    safety_smoothed_force = ApplySafetySlew(target_force, safety_smoothed_force, dt, restricted, now);
    return safety_smoothed_force;
}

