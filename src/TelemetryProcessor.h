#ifndef TELEMETRYPROCESSOR_H
#define TELEMETRYPROCESSOR_H

#include <cmath>
#include <algorithm>
#include <limits>

#if defined(_WIN32) && !defined(DOXYGEN)
#include "lmu_sm_interface/InternalsPlugin.hpp"
#endif

class TelemetryProcessor {
public:

    struct SanitizedLoad {
        double front_load;
        double rear_load;
        double front_grip;
        double rear_grip;
        bool front_load_valid;
        bool rear_load_valid;
        bool front_grip_valid;
        bool rear_grip_valid;
        double confidence;
    };

    struct FallbackState {
        bool front_load_missing;
        bool rear_load_missing;
        bool front_grip_missing;
        bool rear_grip_missing;
        bool susp_force_missing;
        bool lat_force_missing;
        int missing_frames;
    };

    struct KinematicParams {
        double mass_kg;
        double aero_coeff;
        double weight_bias;
        double roll_stiffness;
    };

    struct WeightDistribution {
        double front_left;
        double front_right;
        double rear_left;
        double rear_right;
        double front_bias;
        double cross_weight;
        double total_load;
    };

    struct WeatherData {
        double rain_intensity;
        double track_temp;
        double ambient_temp;
        double grip_modifier;
        bool raining;
    };

    class EMAFilter {
    private:
        mutable double m_state;
        double m_tau;
    public:
        EMAFilter(double tau = 0.1, double initial = 1.0) : m_state(initial), m_tau(tau) {}
        
        double Update(double input, double dt) const {
            double alpha = dt / (m_tau + dt);
            m_state = m_state + alpha * (input - m_state);
            return m_state;
        }
        
        double GetState() const { return m_state; }
        void Reset(double value = 1.0) { m_state = value; }
    };

    static constexpr double MIN_VALID_SUSP_FORCE = 100.0;
    static constexpr double MIN_VALID_TIRE_LOAD = 50.0;
    static constexpr double MAX_VALID_TIRE_LOAD = 15000.0;
    static constexpr double MIN_VALID_GRIP = 0.0;
    static constexpr double MAX_VALID_GRIP = 1.5;
    static constexpr double MIN_VELOCITY = 0.0;
    static constexpr double MAX_VELOCITY = 150.0;

    static bool IsFinite(double val) {
        return std::isfinite(val);
    }

    static bool IsInRange(double val, double min_val, double max_val) {
        return val >= min_val && val <= max_val;
    }

    static double Clamp(double val, double min_val, double max_val) {
        return (std::max)(min_val, (std::min)(val, max_val));
    }

    static SanitizedLoad SanitizeLoad(
        const TelemWheelV01& fl,
        const TelemWheelV01& fr,
        const TelemWheelV01& rl,
        const TelemWheelV01& rr,
        const TelemInfoV01* data,
        double dt,
        const KinematicParams& params = {1100.0, 2.0, 0.55, 0.6}
    ) {
        SanitizedLoad result = {};
        result.confidence = 1.0;

        double susp_force_fl = fl.mSuspForce;
        double susp_force_fr = fr.mSuspForce;

        result.front_load_valid = IsInRange(susp_force_fl, MIN_VALID_TIRE_LOAD, MAX_VALID_TIRE_LOAD) &&
                                  IsInRange(susp_force_fr, MIN_VALID_TIRE_LOAD, MAX_VALID_TIRE_LOAD);

        if (result.front_load_valid) {
            result.front_load = (susp_force_fl + susp_force_fr) / 2.0;
        } else {
            result.front_load = EstimateKinematicLoad(data, 0, params) +
                               EstimateKinematicLoad(data, 1, params);
            result.front_load /= 2.0;
            result.confidence *= 0.7;
        }

        double susp_force_rl = rl.mSuspForce;
        double susp_force_rr = rr.mSuspForce;

        result.rear_load_valid = IsInRange(susp_force_rl, MIN_VALID_TIRE_LOAD, MAX_VALID_TIRE_LOAD) &&
                                 IsInRange(susp_force_rr, MIN_VALID_TIRE_LOAD, MAX_VALID_TIRE_LOAD);

        if (result.rear_load_valid) {
            result.rear_load = (susp_force_rl + susp_force_rr) / 2.0;
        } else {
            result.rear_load = EstimateKinematicLoad(data, 2, params) +
                              EstimateKinematicLoad(data, 3, params);
            result.rear_load /= 2.0;
            result.confidence *= 0.7;
        }

        return result;
    }

    static FallbackState DetectFallbacks(
        const TelemInfoV01* data,
        const FallbackState& prev_state,
        double dt
    ) {
        FallbackState result = {};
        result.missing_frames = prev_state.missing_frames;

        bool load_missing = !IsInRange(data->mWheel[0].mSuspForce, MIN_VALID_TIRE_LOAD, MAX_VALID_TIRE_LOAD);
        bool grip_missing = data->mWheel[0].mGripFract < 0.0001 && data->mWheel[1].mGripFract < 0.0001;

        if (load_missing || grip_missing) {
            result.missing_frames++;
        } else {
            result.missing_frames = 0;
        }

        result.front_load_missing = load_missing;
        result.rear_load_missing = load_missing;
        result.front_grip_missing = grip_missing;
        result.rear_grip_missing = (data->mWheel[2].mGripFract < 0.0001 && data->mWheel[3].mGripFract < 0.0001);
        result.susp_force_missing = load_missing;

        return result;
    }

    static double EstimateKinematicLoad(
        const TelemInfoV01* data,
        int wheel_index,
        const KinematicParams& params = {1100.0, 2.0, 0.55, 0.6}
    ) {
        double velocity_factor = 1.0;
        double speed = std::abs(data->mLocalVel.z);
        if (speed < 10.0) {
            velocity_factor = speed / 10.0;
        }

        bool is_rear = (wheel_index >= 2);
        double bias = is_rear ? params.weight_bias : (1.0 - params.weight_bias);
        double static_weight = (params.mass_kg * 9.81 * bias * velocity_factor) / 2.0;

        double aero_load = params.aero_coeff * (speed * speed);
        double wheel_aero = aero_load / 4.0;

        double long_transfer = (data->mLocalAccel.z / 9.81) * 200.0;
        if (is_rear) long_transfer *= -1.0;

        double lat_transfer = (data->mLocalAccel.x / 9.81) * 200.0 * params.roll_stiffness;
        bool is_left = (wheel_index == 0 || wheel_index == 2);
        if (!is_left) lat_transfer *= -1.0;

        double total_load = static_weight + wheel_aero + long_transfer + lat_transfer;
        return (std::max)(0.0, total_load);
    }

    static WeightDistribution CalculateWeightDistribution(
        const TelemWheelV01& fl,
        const TelemWheelV01& fr,
        const TelemWheelV01& rl,
        const TelemWheelV01& rr,
        const EMAFilter& load_ema,
        double dt
    ) {
        WeightDistribution result = {};
        result.front_left = fl.mTireLoad;
        result.front_right = fr.mTireLoad;
        result.rear_left = rl.mTireLoad;
        result.rear_right = rr.mTireLoad;

        double front_avg = (fl.mTireLoad + fr.mTireLoad) / 2.0;
        double rear_avg = (rl.mTireLoad + rr.mTireLoad) / 2.0;

        double smoothed_front = load_ema.Update(front_avg, dt);
        double smoothed_rear = load_ema.Update(rear_avg, dt);

        result.total_load = smoothed_front + smoothed_rear;
        if (result.total_load > 0.0) {
            result.front_bias = smoothed_front / result.total_load;
        } else {
            result.front_bias = 0.5;
        }

        double left_avg = (fl.mTireLoad + rl.mTireLoad) / 2.0;
        double right_avg = (fr.mTireLoad + rr.mTireLoad) / 2.0;
        result.cross_weight = (left_avg - right_avg) / result.total_load;

        return result;
    }

    static WeatherData ExtractWeather(const ScoringInfoV01* data) {
        WeatherData result = {};
        result.rain_intensity = (double)data->mRaining;
        result.track_temp = (double)data->mTrackTemp - 273.15;
        result.ambient_temp = (double)data->mAmbientTemp - 273.15;
        result.raining = data->mRaining > 0;

        result.grip_modifier = 1.0;

        if (result.raining) {
            result.grip_modifier *= (1.0 - std::min(result.rain_intensity * 0.3, 0.3));
        }

        double temp_factor = 1.0;
        if (result.track_temp < 15.0) {
            temp_factor = 0.85;
        } else if (result.track_temp > 40.0) {
            temp_factor = 0.95;
        }
        result.grip_modifier *= temp_factor;

        return result;
    }

    static double EstimateGripFromSlip(
        double slip_angle,
        double avg_load,
        double optimal_slip = 0.1,
        double max_grip = 1.0
    ) {
        double normalized_slip = std::abs(slip_angle) / optimal_slip;
        normalized_slip = Clamp(normalized_slip, 0.0, 2.0);
        double grip_factor = 1.0 - std::pow(normalized_slip, 2.0) * 0.25;
        grip_factor = Clamp(grip_factor, 0.0, max_grip);
        return grip_factor;
    }

    static bool ValidateTelemetry(const TelemInfoV01* data) {
        if (!IsFinite(data->mLocalVel.x) || !IsFinite(data->mLocalVel.z)) {
            return false;
        }

        double speed = std::abs(data->mLocalVel.z);
        if (!IsInRange(speed, MIN_VELOCITY, MAX_VELOCITY)) {
            return false;
        }

        for (int i = 0; i < 4; i++) {
            if (!IsInRange(data->mWheel[i].mTireLoad, MIN_VALID_TIRE_LOAD, MAX_VALID_TIRE_LOAD)) {
                return false;
            }
            if (!IsInRange(data->mWheel[i].mGripFract, MIN_VALID_GRIP, MAX_VALID_GRIP)) {
                return false;
            }
        }

        return true;
    }
};

#endif
