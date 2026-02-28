#ifndef TOOLTIPS_H
#define TOOLTIPS_H

#include <vector>
#include <string>

namespace Tooltips {

    // General FFB
    inline constexpr const char* DEVICE_SELECT = "Select the DirectInput device to receive Force Feedback signals.\nTypically your steering wheel.";
    inline constexpr const char* DEVICE_RESCAN = "Refresh the list of available DirectInput devices.";
    inline constexpr const char* DEVICE_UNBIND = "Release the current device and disable FFB output.";
    inline constexpr const char* MODE_EXCLUSIVE = "lmuFFB has exclusive control.\nThe game can read steering but cannot send FFB.\nThis prevents 'Double FFB' issues.";
    inline constexpr const char* MODE_SHARED = "lmuFFB is sharing the device.\nEnsure In-Game FFB is disabled\nto avoid LMU reacquiring the device.";
    inline constexpr const char* NO_DEVICE = "Please select your steering wheel from the 'FFB Device' menu above.";
    inline constexpr const char* ALWAYS_ON_TOP = "Keep the lmuFFB window visible over other applications (including the game).";
    inline constexpr const char* SHOW_GRAPHS = "Show real-time physics and output graphs for debugging.\nIncreases window width.";

    // Logging
    inline constexpr const char* LOG_STOP = "Finish recording and save the log file.";
    inline constexpr const char* LOG_REC = "Currently recording high-frequency telemetry data at 100Hz.";
    inline constexpr const char* LOG_MARKER = "Add a timestamped marker to the log file to tag an interesting event.";
    inline constexpr const char* LOG_START = "Start recording high-frequency telemetry and FFB data\nto a CSV file for analysis.";

    // Presets
    inline constexpr const char* PRESET_NAME = "Enter a name for your new user preset.";
    inline constexpr const char* PRESET_SAVE_NEW = "Create a new user preset from the current settings.";
    inline constexpr const char* PRESET_SAVE_CURRENT = "Save modifications to the selected user preset or global calibration.";
    inline constexpr const char* PRESET_RESET = "Revert all settings to factory default (T300 baseline).";
    inline constexpr const char* PRESET_DUPLICATE = "Create a copy of the currently selected preset.";
    inline constexpr const char* PRESET_DELETE = "Remove the selected user preset (builtin presets are protected).";
    inline constexpr const char* PRESET_IMPORT = "Import an external .ini preset file.";
    inline constexpr const char* PRESET_EXPORT = "Export the current preset to an external .ini file.";

    // FFB Settings
    inline constexpr const char* USE_INGAME_FFB = "Recommended for LMU 1.2+. Uses the high-frequency FFB channel\ndirectly from the game.\nMatches the game's internal physics rate for maximum fidelity.";
    inline constexpr const char* INVERT_FFB = "Check this if the wheel pulls away from center instead of aligning.";
    inline constexpr const char* MASTER_GAIN = "Global scale factor for all forces.\n100% = No attenuation.\nReduce if experiencing heavy clipping.";
    inline constexpr const char* WHEELBASE_MAX_TORQUE = "The absolute maximum physical torque your wheelbase can produce\n(e.g., 15.0 for Simagic Alpha, 4.0 for T300).";
    inline constexpr const char* TARGET_RIM_TORQUE = "The maximum force you want to feel in your hands during heavy cornering.";
    inline constexpr const char* MIN_FORCE = "Boosts small forces to overcome mechanical friction/deadzone.";

    // Soft Lock
    inline constexpr const char* SOFT_LOCK_ENABLE = "Provides resistance when the steering wheel reaches\nthe car's maximum steering range.";
    inline constexpr const char* SOFT_LOCK_STIFFNESS = "Intensity of the spring force pushing back from the limit.";
    inline constexpr const char* SOFT_LOCK_DAMPING = "Prevents bouncing and oscillation at the steering limit.";

    // Front Axle
    inline constexpr const char* INGAME_FFB_GAIN = "Scales the native 400Hz In-Game FFB signal.";
    inline constexpr const char* STEERING_SHAFT_GAIN = "Scales the raw steering torque from the physics engine.";
    inline constexpr const char* STEERING_SHAFT_SMOOTHING = "Low Pass Filter applied ONLY to the raw game force.";
    inline constexpr const char* UNDERSTEER_EFFECT = "Scales how much front grip loss reduces steering force.";
    inline constexpr const char* DYNAMIC_WEIGHT = "Scales steering weight based on longitudinal load transfer.\nHeavier under braking, lighter under acceleration.";
    inline constexpr const char* WEIGHT_SMOOTHING = "Filters the Dynamic Weight signal to simulate suspension damping.\nHigher = Smoother weight transfer feel, but less instant.\nRecommended: 0.100s - 0.200s.";
    inline constexpr const char* TORQUE_SOURCE = "Select the telemetry channel for base steering torque.\nShaft Torque: Standard rF2 physics channel (typically 100Hz).\nIn-Game FFB: New LMU high-frequency channel (native 400Hz). RECOMMENDED.\nThis is the actual FFB signal processed by the game engine.";
    inline constexpr const char* PURE_PASSTHROUGH = "Bypasses LMUFFB's internal Understeer and Dynamic Weight modulation\nfor the base steering torque.\nRecommended when using In-Game FFB (400Hz) if you prefer\nthe game's native FFB modulation.";

    // Signal Filtering
    inline constexpr const char* FLATSPOT_SUPPRESSION = "Dynamic Notch Filter that targets wheel rotation frequency.\nSuppresses vibrations caused by tire flatspots.";
    inline constexpr const char* NOTCH_Q = "Quality Factor of the Notch Filter.\nHigher = Narrower bandwidth (surgical removal).\nLower = Wider bandwidth (affects surrounding frequencies).";
    inline constexpr const char* SUPPRESSION_STRENGTH = "How strongly to mute the flatspot vibration.\n1.0 = 100% removal.";
    inline constexpr const char* STATIC_NOISE_FILTER = "Fixed frequency notch filter to remove hardware resonance or specific noise.";
    inline constexpr const char* STATIC_NOTCH_FREQ = "Center frequency to suppress.";
    inline constexpr const char* STATIC_NOTCH_WIDTH = "Bandwidth of the notch filter.\nLarger = Blocks more frequencies around the target.";

    // Rear Axle
    inline constexpr const char* OVERSTEER_BOOST = "Increases the Lateral G (SoP) force when the rear tires lose grip.\nMakes the car feel heavier during a slide, helping you judge the momentum.\nShould build up slightly more gradually than Rear Align Torque,\nreflecting the inertia of the car's mass swinging out.\nIt's a sustained force that tells you about the magnitude of the slide\nTuning Goal: Feel the direction of the counter-steer (Rear Align)\nand the effort required to hold it (Lateral G Boost).";
    inline constexpr const char* LATERAL_G = "Represents Chassis Roll, simulates the weight of the car leaning in the corner.";
    inline constexpr const char* REAR_ALIGN_TORQUE = "Counter-steering force generated by rear tire slip.\nShould build up very quickly after the Yaw Kick, as the slip angle develops.\nThis is the active \"pull.\"\nTuning Goal: Feel the direction of the counter-steer (Rear Align)\nand the effort required to hold it (Lateral G Boost).";
    inline constexpr const char* YAW_KICK = "This is the earliest cue for rear stepping out.\nIt's a sharp, momentary impulse that signals the onset of rotation.\nBased on Yaw Acceleration.";
    inline constexpr const char* YAW_KICK_THRESHOLD = "Minimum yaw acceleration required to trigger the kick.\nIncrease to filter out road noise and small vibrations.";
    inline constexpr const char* YAW_KICK_RESPONSE = "Low Pass Filter for the Yaw Kick signal.\nSmoothes out kick noise.\nLower = Sharper/Faster kick.\nHigher = Duller/Softer kick.";
    inline constexpr const char* GYRO_DAMPING = "Simulates the gyroscopic solidity of the spinning wheels.\nResists rapid steering movements.\nPrevents oscillation and 'Tank Slappers'.\nActs like a steering damper.";
    inline constexpr const char* GYRO_SMOOTH = "Filters the steering velocity signal used for damping.\nReduces noise in the damping effect.\nLow = Crisper damping, High = Smoother.";
    inline constexpr const char* SOP_SMOOTHING = "Filters the Lateral G signal.\nReduces jerkiness in the SoP effect.";
    inline constexpr const char* GRIP_SMOOTHING = "Filters the final estimated grip value.\nUses an adaptive non-linear filter: smooths steady-state noise\nbut maintains zero-latency during rapid grip loss events.\nRecommended: 0.030s - 0.060s.";
    inline constexpr const char* SOP_SCALE = "Multiplies the raw G-force signal before limiting.\nAdjusts the dynamic range of the SoP effect.";

    // Grip Estimation
    inline constexpr const char* SLIP_ANGLE_SMOOTHING = "Applies a time-based filter (LPF) to the Calculated Slip Angle\nused to estimate tire grip.\nSmooths the high fluctuations from lateral and longitudinal velocity,\nespecially over bumps or curbs.\nAffects: Understeer effect, Rear Aligning Torque.";
    inline constexpr const char* CHASSIS_INERTIA = "Simulation time for weight transfer.\nSimulates how fast the suspension settles.\nAffects calculated tire load magnitude.\n25ms = Stiff Race Car.\n50ms = Soft Road Car.";
    inline constexpr const char* OPTIMAL_SLIP_ANGLE = "The slip angle THRESHOLD above which grip loss begins.\nSet this HIGHER than the car's physical peak slip angle.\nRecommended: 0.10 for LMDh/LMP2, 0.12 for GT3.\n\nLower = More sensitive (force drops earlier).\nHigher = More buffer zone before force drops.\n\nNOTE: If the wheel feels too light at the limit, INCREASE this value.\nAffects: Understeer Effect, Lateral G Boost (Slide), Slide Texture.";
    inline constexpr const char* OPTIMAL_SLIP_RATIO = "The longitudinal slip ratio (0.0-1.0) where peak braking/traction occurs.\nTypical: 0.12 - 0.15 (12-15%).\nUsed to estimate grip loss under braking/acceleration.\nAffects: How much braking/acceleration contributes to calculated grip loss.";

    // Slope Detection
    inline constexpr const char* SLOPE_DETECTION_ENABLE = "Replaces static 'Optimal Slip Angle' threshold\nwith dynamic derivative monitoring.\n\nWhen enabled:\nâ€¢ Grip is estimated by tracking the slope of lateral-G vs slip angle\nâ€¢ Automatically adapts to tire temperature, wear, and conditions\nâ€¢ 'Optimal Slip Angle' and 'Optimal Slip Ratio' settings are IGNORED\n\nWhen disabled:\nâ€¢ Uses the static threshold method (default behavior)";
    inline constexpr const char* SLOPE_FILTER_WINDOW = "Savitzky-Golay filter window size (samples).\n\nLarger = Smoother but higher latency\nSmaller = Faster response but noisier\n\nRecommended:\n  Direct Drive: 11-15\n  Belt Drive: 15-21\n  Gear Drive: 21-31\n\nMust be ODD (enforced automatically).";
    inline constexpr const char* SLOPE_SENSITIVITY = "Multiplier for slope-to-grip conversion.\nHigher = More aggressive grip loss detection.\nLower = Smoother, less pronounced effect.";
    inline constexpr const char* SLOPE_THRESHOLD = "Slope value below which grip loss begins.\nMore negative = Later detection (safer).";
    inline constexpr const char* SLOPE_OUTPUT_SMOOTHING = "Time constant for grip factor smoothing.\nPrevents abrupt FFB changes.";
    inline constexpr const char* SLOPE_ALPHA_THRESHOLD = "Confidence threshold for slope detection.\nHigher = Stricter (less noise, potentially slower).";
    inline constexpr const char* SLOPE_DECAY_RATE = "How quickly the grip factor recovers after a slide.\nHigher = Faster recovery.";
    inline constexpr const char* SLOPE_CONFIDENCE_GATE = "Ensures slope changes are statistically significant before applying grip loss.";

    // Braking & Lockup
    inline constexpr const char* LOCKUP_VIBRATION = "Simulates tire judder when wheels are locked under braking.";
    inline constexpr const char* LOCKUP_STRENGTH = "Controls the intensity of the vibration when wheels lock up.\nScales with wheel load and speed.";
    inline constexpr const char* BRAKE_LOAD_CAP = "Scales vibration intensity based on tire load.\nPrevents weak vibrations during high-speed heavy braking.";
    inline constexpr const char* VIBRATION_PITCH = "Scales the frequency of lockup and wheel spin vibrations.\nMatch to your hardware resonance.";
    inline constexpr const char* LOCKUP_GAMMA = "Response Curve Non-Linearity.\n1.0 = Linear.\n>1.0 = Progressive (Starts weak, gets strong fast).\n<1.0 = Aggressive (Starts strong). 2.0=Quadratic, 3.0=Cubic (Late/Sharp)";
    inline constexpr const char* LOCKUP_START_PCT = "Slip percentage where vibration begins.\n1.0% = Immediate feedback.\n5.0% = Only on deep lock.";
    inline constexpr const char* LOCKUP_FULL_PCT = "Slip percentage where vibration reaches maximum intensity.";
    inline constexpr const char* LOCKUP_PREDICTION_SENS = "Angular Deceleration Threshold.\nHow aggressively the system predicts a lockup before it physically occurs.\nLower = More sensitive (triggers earlier).\nHigher = Less sensitive.";
    inline constexpr const char* LOCKUP_BUMP_REJECT = "Suspension velocity threshold.\nDisables prediction on bumpy surfaces to prevent false positives.\nIncrease for bumpy tracks (Sebring).";
    inline constexpr const char* LOCKUP_REAR_BOOST = "Multiplies amplitude when rear wheels lock harder than front wheels.\nHelps distinguish rear locking (dangerous) from front locking (understeer).";
    inline constexpr const char* ABS_PULSE = "Simulates the pulsing of an ABS system.\nInjects high-frequency pulse when ABS modulates pressure.";
    inline constexpr const char* ABS_PULSE_GAIN = "Intensity of the ABS pulse.";
    inline constexpr const char* ABS_PULSE_FREQ = "Rate of the ABS pulse oscillation.";

    // Tactile Textures
    inline constexpr const char* TEXTURE_LOAD_CAP = "Safety Limiter specific to Road and Slide textures.\nPrevents violent shaking when under high downforce or compression.\nONLY affects Road Details and Slide Rumble.";
    inline constexpr const char* SLIDE_RUMBLE = "Vibration proportional to tire sliding/scrubbing velocity.";
    inline constexpr const char* SLIDE_GAIN = "Intensity of the scrubbing vibration.";
    inline constexpr const char* SLIDE_PITCH = "Frequency multiplier for the scrubbing sound/feel.\nHigher = Screeching.\nLower = Grinding.";
    inline constexpr const char* ROAD_DETAILS = "Vibration derived from high-frequency suspension movement.\nFeels road surface, cracks, and bumps.";
    inline constexpr const char* ROAD_GAIN = "Intensity of road details.";
    inline constexpr const char* SPIN_VIBRATION = "Vibration when wheels lose traction under acceleration (Wheel Spin).";
    inline constexpr const char* SPIN_STRENGTH = "Intensity of the wheel spin vibration.";
    inline constexpr const char* SPIN_PITCH = "Scales the frequency of the wheel spin vibration.";
    inline constexpr const char* SCRUB_DRAG = "Constant resistance force when pushing tires laterally (Understeer drag).\nAdds weight to the wheel when scrubbing.";
    inline constexpr const char* BOTTOMING_LOGIC = "Algorithm for detecting suspension bottoming.\nScraping = Ride height based.\nSusp Spike = Force rate based.";

    // Advanced
    inline constexpr const char* MUTE_BELOW = "The speed below which all haptic vibrations (Road, Slide, Lockup, Spin)\nare completely muted to prevent idle shaking.";
    inline constexpr const char* FULL_ABOVE = "The speed above which all haptic vibrations reach\ntheir full configured strength.";
    inline constexpr const char* AUTO_START_LOGGING = "Automatically start telemetry logging when entering a driving session.";
    inline constexpr const char* LOG_PATH = "Directory where .csv telemetry logs will be saved.";

    // Debug Plots
    inline constexpr const char* PLOT_SELECTED_TORQUE = "The torque value currently being used as the base for FFB calculations.";
    inline constexpr const char* PLOT_SHAFT_TORQUE = "Standard rF2 physics channel (typically 100Hz).";
    inline constexpr const char* PLOT_INGAME_FFB = "New LMU high-frequency channel (native 400Hz).";

    // GuiWidgets fixed tooltips
    inline constexpr const char* FINE_TUNE = "Fine Tune: Arrow Keys | Exact: Ctrl+Click";

    inline const std::vector<const char*> ALL = {
        DEVICE_SELECT, DEVICE_RESCAN, DEVICE_UNBIND, MODE_EXCLUSIVE, MODE_SHARED, NO_DEVICE, ALWAYS_ON_TOP, SHOW_GRAPHS,
        LOG_STOP, LOG_REC, LOG_MARKER, LOG_START,
        PRESET_NAME, PRESET_SAVE_NEW, PRESET_SAVE_CURRENT, PRESET_RESET, PRESET_DUPLICATE, PRESET_DELETE, PRESET_IMPORT, PRESET_EXPORT,
        USE_INGAME_FFB, INVERT_FFB, MASTER_GAIN, WHEELBASE_MAX_TORQUE, TARGET_RIM_TORQUE, MIN_FORCE,
        SOFT_LOCK_ENABLE, SOFT_LOCK_STIFFNESS, SOFT_LOCK_DAMPING,
        INGAME_FFB_GAIN, STEERING_SHAFT_GAIN, STEERING_SHAFT_SMOOTHING, UNDERSTEER_EFFECT, DYNAMIC_WEIGHT, WEIGHT_SMOOTHING, TORQUE_SOURCE, PURE_PASSTHROUGH,
        FLATSPOT_SUPPRESSION, NOTCH_Q, SUPPRESSION_STRENGTH, STATIC_NOISE_FILTER, STATIC_NOTCH_FREQ, STATIC_NOTCH_WIDTH,
        OVERSTEER_BOOST, LATERAL_G, REAR_ALIGN_TORQUE, YAW_KICK, YAW_KICK_THRESHOLD, YAW_KICK_RESPONSE, GYRO_DAMPING, GYRO_SMOOTH, SOP_SMOOTHING, GRIP_SMOOTHING, SOP_SCALE,
        SLIP_ANGLE_SMOOTHING, CHASSIS_INERTIA, OPTIMAL_SLIP_ANGLE, OPTIMAL_SLIP_RATIO,
        SLOPE_DETECTION_ENABLE, SLOPE_FILTER_WINDOW, SLOPE_SENSITIVITY, SLOPE_THRESHOLD, SLOPE_OUTPUT_SMOOTHING, SLOPE_ALPHA_THRESHOLD, SLOPE_DECAY_RATE, SLOPE_CONFIDENCE_GATE,
        LOCKUP_VIBRATION, LOCKUP_STRENGTH, BRAKE_LOAD_CAP, VIBRATION_PITCH, LOCKUP_GAMMA, LOCKUP_START_PCT, LOCKUP_FULL_PCT, LOCKUP_PREDICTION_SENS, LOCKUP_BUMP_REJECT, LOCKUP_REAR_BOOST, ABS_PULSE, ABS_PULSE_GAIN, ABS_PULSE_FREQ,
        TEXTURE_LOAD_CAP, SLIDE_RUMBLE, SLIDE_GAIN, SLIDE_PITCH, ROAD_DETAILS, ROAD_GAIN, SPIN_VIBRATION, SPIN_STRENGTH, SPIN_PITCH, SCRUB_DRAG, BOTTOMING_LOGIC,
        MUTE_BELOW, FULL_ABOVE, AUTO_START_LOGGING, LOG_PATH,
        PLOT_SELECTED_TORQUE, PLOT_SHAFT_TORQUE, PLOT_INGAME_FFB,
        FINE_TUNE
    };
}

#endif // TOOLTIPS_H
