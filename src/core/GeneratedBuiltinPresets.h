// AUTO-GENERATED FILE - DO NOT EDIT
#pragma once
#include <string_view>
#include <map>

inline const std::map<std::string_view, std::string_view> BUILTIN_PRESETS = {
    {"Test_Textures_Only", R"PRESET(name = "Test: Textures Only"
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
sop_scale = 0.0
sop_smoothing_factor = 0.0
rear_align_effect = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
[Braking]
lockup_enabled = true
lockup_gain = 1.0
[Vibration]
spin_enabled = true
spin_gain = 1.0
slide_enabled = true
slide_gain = 0.39
road_enabled = true
road_gain = 1.0
bottoming_enabled = true
bottoming_gain = 1.0
bottoming_method = 0
)PRESET"},
    {"Guide_Slide_Texture_(Scrub)", R"PRESET(name = "Guide: Slide Texture (Scrub)"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_smoothing_factor = 0.0
[Vibration]
slide_enabled = true
slide_gain = 0.39
slide_freq = 1.0
scrub_drag_gain = 1.0
spin_enabled = false
spin_gain = 0.0
road_enabled = false
road_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[Braking]
lockup_enabled = false
lockup_gain = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"GM_DD_21_Nm_(Moza_R21_Ultra)", R"PRESET(name = "GM DD 21 Nm (Moza R21 Ultra)"
[General]
gain = 1.454
wheelbase_max_nm = 21.0
target_rim_nm = 12.0
min_force = 0.0
[FrontAxle]
steering_shaft_gain = 1.989
steering_shaft_smoothing = 0.0
understeer_effect = 0.638
flatspot_suppression = true
notch_q = 0.57
flatspot_strength = 1.0
static_notch_enabled = false
static_notch_freq = 11.0
static_notch_width = 2.0
[RearAxle]
oversteer_boost = 0.0
sop_effect = 0.0
rear_align_effect = 0.29
sop_yaw_gain = 0.0
yaw_kick_threshold = 0.0
yaw_accel_smoothing = 0.015
sop_smoothing_factor = 0.0
sop_scale = 0.89
[GripEstimation]
slip_angle_smoothing = 0.002
chassis_inertia_smoothing = 0.0
optimal_slip_angle = 0.1
optimal_slip_ratio = 0.12
[Braking]
lockup_enabled = true
lockup_gain = 0.977
brake_load_cap = 81.0
lockup_freq_scale = 1.0
lockup_gamma = 1.0
lockup_start_pct = 1.0
lockup_full_pct = 7.5
lockup_prediction_sens = 10.0
lockup_bump_reject = 0.1
lockup_rear_boost = 1.0
abs_pulse_enabled = false
abs_gain = 2.1
abs_freq = 25.5
[Vibration]
texture_load_cap = 1.5
slide_enabled = false
slide_gain = 0.0
slide_freq = 1.47
road_enabled = true
road_gain = 0.0
spin_enabled = true
spin_gain = 0.462185
spin_freq_scale = 1.8
scrub_drag_gain = 0.333
bottoming_enabled = true
bottoming_gain = 1.0
bottoming_method = 1
[Advanced]
gyro_gain = 0.0
gyro_smoothing = 0.0
understeer_affects_sop = false
road_fallback_scale = 0.05
speed_gate_lower = 1.0
speed_gate_upper = 5.0
)PRESET"},
    {"Guide_Understeer_(Front_Grip)", R"PRESET(name = "Guide: Understeer (Front Grip)"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.61
[RearAxle]
sop_effect = 0.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_yaw_gain = 0.0
sop_smoothing_factor = 0.0
[Advanced]
gyro_gain = 0.0
[Braking]
lockup_enabled = false
lockup_gain = 0.0
[Vibration]
spin_enabled = false
spin_gain = 0.0
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"Test_SoP_Base_Only", R"PRESET(name = "Test: SoP Base Only"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.08
sop_smoothing_factor = 0.0
rear_align_effect = 0.0
sop_yaw_gain = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
[Vibration]
slide_enabled = false
slide_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
)PRESET"},
    {"Guide_Gyroscopic_Damping", R"PRESET(name = "Guide: Gyroscopic Damping"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_yaw_gain = 0.0
sop_smoothing_factor = 0.0
[Advanced]
gyro_gain = 1.0
[Braking]
lockup_enabled = false
lockup_gain = 0.0
[Vibration]
spin_enabled = false
spin_gain = 0.0
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"Guide_Oversteer_(Rear_Grip)", R"PRESET(name = "Guide: Oversteer (Rear Grip)"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.08
sop_scale = 1.0
rear_align_effect = 0.90
oversteer_boost = 0.65
sop_yaw_gain = 0.0
sop_smoothing_factor = 0.0
[Advanced]
gyro_gain = 0.0
[Braking]
lockup_enabled = false
lockup_gain = 0.0
[Vibration]
spin_enabled = false
spin_gain = 0.0
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"Default", R"PRESET(name = "Default"
[General]
# Standard defaults
)PRESET"},
    {"Simucube_2_SportProUltimate", R"PRESET(name = "Simucube 2 Sport/Pro/Ultimate"
[General]
# Standard defaults
)PRESET"},
    {"Test_Understeer_Only", R"PRESET(name = "Test: Understeer Only"
[FrontAxle]
understeer_effect = 0.61
[RearAxle]
sop_effect = 0.0
sop_scale = 1.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_yaw_gain = 0.0
sop_smoothing_factor = 0.0
[Advanced]
gyro_gain = 0.0
speed_gate_lower = 0.0
speed_gate_upper = 0.0
[Vibration]
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
spin_enabled = false
spin_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[Braking]
lockup_enabled = false
lockup_gain = 0.0
abs_pulse_enabled = false
abs_gain = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
optimal_slip_angle = 0.1
optimal_slip_ratio = 0.12
)PRESET"},
    {"Test_SoP_Only", R"PRESET(name = "Test: SoP Only"
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.08
sop_scale = 1.0
sop_smoothing_factor = 0.0
rear_align_effect = 0.0
sop_yaw_gain = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
[Vibration]
slide_enabled = false
slide_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
)PRESET"},
    {"Test_No_Effects", R"PRESET(name = "Test: No Effects"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
sop_smoothing_factor = 0.0
rear_align_effect = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
[Vibration]
slide_enabled = false
slide_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
)PRESET"},
    {"Guide_SoP_Yaw_(Kick)", R"PRESET(name = "Guide: SoP Yaw (Kick)"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_yaw_gain = 5.0
sop_smoothing_factor = 0.0
[Advanced]
gyro_gain = 0.0
[Braking]
lockup_enabled = false
lockup_gain = 0.0
[Vibration]
spin_enabled = false
spin_gain = 0.0
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"GT3_DD_15_Nm_(Simagic_Alpha)", R"PRESET(name = "GT3 DD 15 Nm (Simagic Alpha)"
[General]
gain = 1.0
wheelbase_max_nm = 15.0
target_rim_nm = 10.0
min_force = 0.0
[FrontAxle]
steering_shaft_gain = 1.0
steering_shaft_smoothing = 0.0
understeer_effect = 1.0
flatspot_suppression = false
notch_q = 2.0
flatspot_strength = 1.0
static_notch_enabled = false
static_notch_freq = 11.0
static_notch_width = 2.0
[RearAxle]
oversteer_boost = 2.52101
sop_effect = 1.666
rear_align_effect = 0.666
sop_yaw_gain = 0.333
yaw_kick_threshold = 0.0
yaw_accel_smoothing = 0.001
sop_smoothing_factor = 0.0
sop_scale = 1.98
[GripEstimation]
slip_angle_smoothing = 0.002
chassis_inertia_smoothing = 0.012
optimal_slip_angle = 0.1
optimal_slip_ratio = 0.12
[Braking]
lockup_enabled = true
lockup_gain = 0.37479
brake_load_cap = 2.0
lockup_freq_scale = 1.0
lockup_gamma = 1.0
lockup_start_pct = 1.0
lockup_full_pct = 7.5
lockup_prediction_sens = 10.0
lockup_bump_reject = 0.1
lockup_rear_boost = 1.0
abs_pulse_enabled = false
abs_gain = 2.1
abs_freq = 25.5
[Vibration]
texture_load_cap = 1.5
slide_enabled = false
slide_gain = 0.226562
slide_freq = 1.47
road_enabled = true
road_gain = 0.0
spin_enabled = true
spin_gain = 0.462185
spin_freq_scale = 1.8
scrub_drag_gain = 0.333
bottoming_enabled = true
bottoming_gain = 1.0
bottoming_method = 1
[Advanced]
gyro_gain = 0.0
gyro_smoothing = 0.0
understeer_affects_sop = false
road_fallback_scale = 0.05
speed_gate_lower = 1.0
speed_gate_upper = 5.0
)PRESET"},
    {"LMPxHY_DD_15_Nm_(Simagic_Alpha)", R"PRESET(name = "LMPx/HY DD 15 Nm (Simagic Alpha)"
[General]
gain = 1.0
wheelbase_max_nm = 15.0
target_rim_nm = 10.0
min_force = 0.0
[FrontAxle]
steering_shaft_gain = 1.0
steering_shaft_smoothing = 0.0
understeer_effect = 1.0
flatspot_suppression = false
notch_q = 2.0
flatspot_strength = 1.0
static_notch_enabled = false
static_notch_freq = 11.0
static_notch_width = 2.0
[RearAxle]
oversteer_boost = 2.52101
sop_effect = 1.666
rear_align_effect = 0.666
sop_yaw_gain = 0.333
yaw_kick_threshold = 0.0
yaw_accel_smoothing = 0.003
sop_smoothing_factor = 0.0
sop_scale = 1.59
[GripEstimation]
slip_angle_smoothing = 0.003
chassis_inertia_smoothing = 0.019
optimal_slip_angle = 0.12
optimal_slip_ratio = 0.12
[Braking]
lockup_enabled = true
lockup_gain = 0.37479
brake_load_cap = 2.0
lockup_freq_scale = 1.0
lockup_gamma = 1.0
lockup_start_pct = 1.0
lockup_full_pct = 7.5
lockup_prediction_sens = 10.0
lockup_bump_reject = 0.1
lockup_rear_boost = 1.0
abs_pulse_enabled = false
abs_gain = 2.1
abs_freq = 25.5
[Vibration]
texture_load_cap = 1.5
slide_enabled = false
slide_gain = 0.226562
slide_freq = 1.47
road_enabled = true
road_gain = 0.0
spin_enabled = true
spin_gain = 0.462185
spin_freq_scale = 1.8
scrub_drag_gain = 0.333
bottoming_enabled = true
bottoming_gain = 1.0
bottoming_method = 1
[Advanced]
gyro_gain = 0.0
gyro_smoothing = 0.003
understeer_affects_sop = false
road_fallback_scale = 0.05
speed_gate_lower = 1.0
speed_gate_upper = 5.0
)PRESET"},
    {"Guide_Traction_Loss_(Spin)", R"PRESET(name = "Guide: Traction Loss (Spin)"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_smoothing_factor = 0.0
[Vibration]
spin_enabled = true
spin_gain = 1.0
lockup_enabled = false
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"T300_v0.7.164", R"PRESET(name = "T300 v0.7.164"
app_version = "0.7.165"
[General]
gain = 0.559471
wheelbase_max_nm = 25.1
target_rim_nm = 24.5
min_force = 0.02
[FrontAxle]
steering_shaft_gain = 0.955947
ingame_ffb_gain = 1.0
steering_shaft_smoothing = 0.0
understeer_effect = 1.0
torque_source = 0
torque_passthrough = false
flatspot_suppression = false
notch_q = 2.0
flatspot_strength = 1.0
static_notch_enabled = false
static_notch_freq = 11.0
static_notch_width = 2.0
[RearAxle]
oversteer_boost = 0.0
sop_effect = 0.0
rear_align_effect = 0.828194
sop_yaw_gain = 0.418502
yaw_kick_threshold = 1.01
unloaded_yaw_gain = 1.0
unloaded_yaw_threshold = 0.0
unloaded_yaw_sens = 0.1
unloaded_yaw_gamma = 0.1
unloaded_yaw_punch = 0.0
power_yaw_gain = 1.0
power_yaw_threshold = 0.0
power_slip_threshold = 0.0618062
power_yaw_gamma = 0.2
power_yaw_punch = 0.0
yaw_accel_smoothing = 0.001
sop_smoothing_factor = 0.0
sop_scale = 1.0
[LoadForces]
long_load_effect = 2.68722
long_load_smoothing = 0.15
long_load_transform = 0
lat_load_effect = 2.81938
lat_load_transform = 2
[GripEstimation]
grip_smoothing_steady = 0.05
grip_smoothing_fast = 0.005
grip_smoothing_sensitivity = 0.1
slip_angle_smoothing = 0.002
chassis_inertia_smoothing = 0.0
optimal_slip_angle = 0.1
optimal_slip_ratio = 0.12
[Advanced]
gyro_gain = 0.0
gyro_smoothing = 0.0
understeer_affects_sop = false
road_fallback_scale = 0.05
soft_lock_enabled = false
soft_lock_stiffness = 20.0
soft_lock_damping = 0.5
rest_api_enabled = true
rest_api_port = 6397
speed_gate_lower = 1.0
speed_gate_upper = 5.0
[SlopeDetection]
enabled = false
sg_window = 15
sensitivity = 0.5
smoothing_tau = 0.04
min_threshold = -0.3
max_threshold = -2.0
alpha_threshold = 0.02
decay_rate = 5.0
confidence_enabled = true
g_slew_limit = 50.0
use_torque = true
torque_sensitivity = 0.5
confidence_max_rate = 0.1
[Braking]
lockup_enabled = true
lockup_gain = 3.0
brake_load_cap = 10.0
lockup_freq_scale = 1.02
lockup_gamma = 0.1
lockup_start_pct = 1.0
lockup_full_pct = 5.0
lockup_prediction_sens = 10.0
lockup_bump_reject = 0.1
lockup_rear_boost = 10.0
abs_pulse_enabled = false
abs_gain = 2.0
abs_freq = 25.5
[Vibration]
texture_load_cap = 1.5
slide_enabled = false
slide_gain = 0.226562
slide_freq = 1.0
road_enabled = false
road_gain = 0.0
vibration_gain = 1.0
spin_enabled = false
spin_gain = 0.5
spin_freq_scale = 1.0
scrub_drag_gain = 0.0
bottoming_enabled = true
bottoming_gain = 1.0
bottoming_method = 0
[Safety]
window_duration = 0.0
gain_reduction = 0.3
smoothing_tau = 0.2
spike_detection_threshold = 500.0
immediate_spike_threshold = 1500.0
slew_full_scale_time_s = 1.0
stutter_safety_enabled = false
stutter_threshold = 1.5
)PRESET"},
    {"Test_Yaw_Kick_Only", R"PRESET(name = "Test: Yaw Kick Only"
[RearAxle]
sop_yaw_gain = 0.386555
yaw_kick_threshold = 1.68
yaw_accel_smoothing = 0.005
sop_effect = 0.0
sop_scale = 1.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_smoothing_factor = 0.0
[FrontAxle]
understeer_effect = 0.0
[Advanced]
gyro_gain = 0.0
[Vibration]
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
spin_enabled = false
spin_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[Braking]
lockup_enabled = false
lockup_gain = 0.0
abs_pulse_enabled = false
abs_gain = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"Test_Slide_Texture_Only", R"PRESET(name = "Test: Slide Texture Only"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
sop_smoothing_factor = 0.0
rear_align_effect = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
[Vibration]
slide_enabled = true
slide_gain = 0.39
slide_freq = 1.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
)PRESET"},
    {"Logitech_G25G27G29G920", R"PRESET(name = "Logitech G25/G27/G29/G920"
[General]
# Standard defaults for Logitech gear-driven wheels
)PRESET"},
    {"GM_+_Yaw_Kick_DD_21_Nm_(Moza_R21_Ultra)", R"PRESET(name = "GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)"
[General]
gain = 1.454
wheelbase_max_nm = 21.0
target_rim_nm = 12.0
min_force = 0.0
[FrontAxle]
steering_shaft_gain = 1.989
steering_shaft_smoothing = 0.0
understeer_effect = 0.638
flatspot_suppression = true
notch_q = 0.57
flatspot_strength = 1.0
static_notch_enabled = false
static_notch_freq = 11.0
static_notch_width = 2.0
[RearAxle]
oversteer_boost = 0.0
sop_effect = 0.0
rear_align_effect = 0.29
sop_yaw_gain = 0.333
yaw_kick_threshold = 0.0
yaw_accel_smoothing = 0.003
sop_smoothing_factor = 0.0
sop_scale = 0.89
[GripEstimation]
slip_angle_smoothing = 0.002
chassis_inertia_smoothing = 0.0
optimal_slip_angle = 0.1
optimal_slip_ratio = 0.12
[Braking]
lockup_enabled = true
lockup_gain = 0.977
brake_load_cap = 81.0
lockup_freq_scale = 1.0
lockup_gamma = 1.0
lockup_start_pct = 1.0
lockup_full_pct = 7.5
lockup_prediction_sens = 10.0
lockup_bump_reject = 0.1
lockup_rear_boost = 1.0
abs_pulse_enabled = false
abs_gain = 2.1
abs_freq = 25.5
[Vibration]
texture_load_cap = 1.5
slide_enabled = false
slide_gain = 0.0
slide_freq = 1.47
road_enabled = true
road_gain = 0.0
spin_enabled = true
spin_gain = 0.462185
spin_freq_scale = 1.8
scrub_drag_gain = 0.333
bottoming_enabled = true
bottoming_gain = 1.0
bottoming_method = 1
[Advanced]
gyro_gain = 0.0
gyro_smoothing = 0.0
understeer_affects_sop = false
road_fallback_scale = 0.05
speed_gate_lower = 1.0
speed_gate_upper = 5.0
)PRESET"},
    {"Fanatec_Podium_DD1DD2", R"PRESET(name = "Fanatec Podium DD1/DD2"
[General]
# Standard defaults
)PRESET"},
    {"Thrustmaster_T-GTT-GT_II", R"PRESET(name = "Thrustmaster T-GT/T-GT II"
[General]
# Standard defaults
)PRESET"},
    {"Thrustmaster_TS-PCTS-XW", R"PRESET(name = "Thrustmaster TS-PC/TS-XW"
[General]
# Standard defaults
)PRESET"},
    {"Test_Rear_Align_Torque_Only", R"PRESET(name = "Test: Rear Align Torque Only"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
sop_smoothing_factor = 0.0
rear_align_effect = 0.90
sop_yaw_gain = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
[Vibration]
slide_enabled = false
slide_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
)PRESET"},
    {"Moza_R5R9R16R21", R"PRESET(name = "Moza R5/R9/R16/R21"
[General]
# Standard defaults
)PRESET"},
    {"Test_Game_Base_FFB_Only", R"PRESET(name = "Test: Game Base FFB Only"
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
sop_scale = 1.0
sop_smoothing_factor = 0.0
rear_align_effect = 0.0
[GripEstimation]
slip_angle_smoothing = 0.015
[Vibration]
slide_enabled = false
slide_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
)PRESET"},
    {"Fanatec_CSL_DD__GT_DD_Pro", R"PRESET(name = "Fanatec CSL DD / GT DD Pro"
[General]
# Standard defaults
)PRESET"},
    {"Simagic_AlphaAlpha_MiniAlpha_U", R"PRESET(name = "Simagic Alpha/Alpha Mini/Alpha U"
[General]
# Standard defaults
)PRESET"},
    {"Guide_Braking_Lockup", R"PRESET(name = "Guide: Braking Lockup"
[General]
gain = 1.0
[FrontAxle]
understeer_effect = 0.0
[RearAxle]
sop_effect = 0.0
oversteer_boost = 0.0
rear_align_effect = 0.0
sop_smoothing_factor = 0.0
[Braking]
lockup_enabled = true
lockup_gain = 1.0
[Vibration]
spin_enabled = false
spin_gain = 0.0
slide_enabled = false
slide_gain = 0.0
road_enabled = false
road_gain = 0.0
scrub_drag_gain = 0.0
bottoming_enabled = false
bottoming_gain = 0.0
bottoming_method = 0
[GripEstimation]
slip_angle_smoothing = 0.015
)PRESET"},
    {"Thrustmaster_T300TX", R"PRESET(name = "Thrustmaster T300/TX"
[General]
min_force = 0.08
[FrontAxle]
steering_shaft_smoothing = 0.15
)PRESET"}
};
