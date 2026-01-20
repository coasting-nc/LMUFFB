#ifndef CONFIG_H
#define CONFIG_H

#include "FFBEngine.h"
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct Preset {
  std::string name;
  bool is_builtin = false;
  bool invert_force = true;
  float gain = 1.0f;
  float max_torque_ref = 100.0f;
  float min_force = 0.0f;
  float steering_shaft_gain = 1.0f;
  float steering_shaft_smoothing = 0.0f;
  float understeer = 1.0f; // New scale: 0.0-2.0, where 1.0 = proportional
  int base_force_mode = 0; // 0=Native
  bool flatspot_suppression = false;
  float notch_q = 2.0f;
  float flatspot_strength = 1.0f;
  bool static_notch_enabled = false;
  float static_notch_freq = 11.0f;
  float static_notch_width = 2.0f;
  float oversteer_boost = 2.52101f;
  float sop = 1.666f;
  float rear_align_effect = 0.666f;
  float sop_yaw_gain = 0.333f;
  float yaw_kick_threshold = 0.0f;
  float yaw_smoothing = 0.001f;
  float gyro_gain = 0.0f;
  float gyro_smoothing = 0.0f;
  float sop_smoothing = 1.0f;
  float sop_scale = 1.0f;
  bool understeer_affects_sop = false; // Planned: Understeer modulation of SoP
  float slip_smoothing = 0.002f;
  float chassis_smoothing = 0.0f;
  float optimal_slip_angle = 0.1f;
  float optimal_slip_ratio = 0.12f;
  bool lockup_enabled = true;
  float lockup_gain = 0.37479f;
  float brake_load_cap = 2.0f;
  float lockup_freq_scale = 1.02f;
  float lockup_gamma = 0.1f;
  float lockup_start_pct = 1.0f;
  float lockup_full_pct = 5.0f;
  float lockup_prediction_sens = 10.0f;
  float lockup_bump_reject = 0.1f;
  float lockup_rear_boost = 10.0f;
  bool abs_pulse_enabled = false;
  float abs_gain = 2.0f;
  float abs_freq = 25.5f;
  float texture_load_cap = 1.5f;
  bool slide_enabled = false;
  float slide_gain = 0.226562f;
  float slide_freq = 1.0f;
  bool road_enabled = true;
  float road_gain = 0.0f;
  float road_fallback_scale = 0.05f;
  bool spin_enabled = true;
  float spin_gain = 0.5f;
  float spin_freq_scale = 1.0f;
  float scrub_drag_gain = 0.0f;
  int bottoming_method = 0;
  float speed_gate_lower = 1.0f;
  float speed_gate_upper = 5.0f;

  // Fluent Setters (The "Python Dictionary" feel)
  Preset &SetGain(float v) {
    gain = v;
    return *this;
  }
  Preset &SetUndersteer(float v) {
    understeer = v;
    return *this;
  }
  Preset &SetSoP(float v) {
    sop = v;
    return *this;
  }
  Preset &SetSoPScale(float v) {
    sop_scale = v;
    return *this;
  }
  Preset &SetSmoothing(float v) {
    sop_smoothing = v;
    return *this;
  }
  Preset &SetMinForce(float v) {
    min_force = v;
    return *this;
  }
  Preset &SetOversteer(float v) {
    oversteer_boost = v;
    return *this;
  }
  Preset &SetSlipSmoothing(float v) {
    slip_smoothing = v;
    return *this;
  }

  Preset &SetLockup(bool enabled, float g, float start = 5.0f,
                    float full = 15.0f, float boost = 1.5f) {
    lockup_enabled = enabled;
    lockup_gain = g;
    lockup_start_pct = start;
    lockup_full_pct = full;
    lockup_rear_boost = boost;
    return *this;
  }
  Preset &SetBrakeCap(float v) {
    brake_load_cap = v;
    return *this;
  }
  Preset &SetSpin(bool enabled, float g, float scale = 1.0f) {
    spin_enabled = enabled;
    spin_gain = g;
    spin_freq_scale = scale;
    return *this;
  }
  Preset &SetSlide(bool enabled, float g, float f = 1.0f) {
    slide_enabled = enabled;
    slide_gain = g;
    slide_freq = f;
    return *this;
  }
  Preset &SetRoad(bool enabled, float g) {
    road_enabled = enabled;
    road_gain = g;
    return *this;
  }

  Preset &SetInvert(bool v) {
    invert_force = v;
    return *this;
  }
  Preset &SetMaxTorque(float v) {
    max_torque_ref = v;
    return *this;
  }

  Preset &SetBottoming(int method) {
    bottoming_method = method;
    return *this;
  }
  Preset &SetScrub(float v) {
    scrub_drag_gain = v;
    return *this;
  }
  Preset &SetRearAlign(float v) {
    rear_align_effect = v;
    return *this;
  }
  Preset &SetSoPYaw(float v) {
    sop_yaw_gain = v;
    return *this;
  }
  Preset &SetGyro(float v) {
    gyro_gain = v;
    return *this;
  }

  Preset &SetShaftGain(float v) {
    steering_shaft_gain = v;
    return *this;
  }
  Preset &SetBaseMode(int v) {
    base_force_mode = v;
    return *this;
  }
  Preset &SetFlatspot(bool enabled, float strength = 1.0f, float q = 2.0f) {
    flatspot_suppression = enabled;
    flatspot_strength = strength;
    notch_q = q;
    return *this;
  }

  Preset &SetStaticNotch(bool enabled, float freq, float width = 2.0f) {
    static_notch_enabled = enabled;
    static_notch_freq = freq;
    static_notch_width = width;
    return *this;
  }
  Preset &SetYawKickThreshold(float v) {
    yaw_kick_threshold = v;
    return *this;
  }
  Preset &SetSpeedGate(float lower, float upper) {
    speed_gate_lower = lower;
    speed_gate_upper = upper;
    return *this;
  }

  Preset &SetOptimalSlip(float angle, float ratio) {
    optimal_slip_angle = angle;
    optimal_slip_ratio = ratio;
    return *this;
  }
  Preset &SetShaftSmoothing(float v) {
    steering_shaft_smoothing = v;
    return *this;
  }

  Preset &SetGyroSmoothing(float v) {
    gyro_smoothing = v;
    return *this;
  }
  Preset &SetYawSmoothing(float v) {
    yaw_smoothing = v;
    return *this;
  }
  Preset &SetChassisSmoothing(float v) {
    chassis_smoothing = v;
    return *this;
  }

  // Advanced Braking (v0.6.0)
  // ⚠️ IMPORTANT: Default parameters (abs_f, lockup_f) must match Config.h
  // defaults! When changing Config.h defaults, update these values to match.
  // Current: abs_f=25.5, lockup_f=1.02 (GT3 DD 15 Nm defaults - v0.6.35)
  Preset &SetAdvancedBraking(float gamma, float sens, float bump, bool abs,
                             float abs_g, float abs_f = 25.5f,
                             float lockup_f = 1.02f) {
    lockup_gamma = gamma;
    lockup_prediction_sens = sens;
    lockup_bump_reject = bump;
    abs_pulse_enabled = abs;
    abs_gain = abs_g;
    abs_freq = abs_f;
    lockup_freq_scale = lockup_f;
    return *this;
  }

  // 4. method to apply defaults to FFBEngine (Single Source of Truth)
  // This is called by FFBEngine constructor to initialize with T300 defaults
  void ApplyDefaultsToEngine(FFBEngine &engine) {
    Preset defaults; // Uses default member initializers (T300 values)
    defaults.Apply(engine);
  }

  // Apply this preset to an engine instance
  void Apply(FFBEngine &engine) const {
    engine.m_gain = gain;
    engine.m_understeer_effect = understeer;
    engine.m_sop_effect = sop;
    engine.m_sop_scale = sop_scale;
    engine.m_sop_smoothing_factor = sop_smoothing;
    engine.m_slip_angle_smoothing = slip_smoothing;
    engine.m_min_force = min_force;
    engine.m_oversteer_boost = oversteer_boost;
    engine.m_lockup_enabled = lockup_enabled;
    engine.m_lockup_gain = lockup_gain;
    engine.m_lockup_start_pct = lockup_start_pct;
    engine.m_lockup_full_pct = lockup_full_pct;
    engine.m_lockup_rear_boost = lockup_rear_boost;
    engine.m_lockup_gamma = lockup_gamma;
    engine.m_lockup_prediction_sens = lockup_prediction_sens;
    engine.m_lockup_bump_reject = lockup_bump_reject;
    engine.m_brake_load_cap = brake_load_cap;
    engine.m_texture_load_cap = texture_load_cap; // NEW v0.6.25
    engine.m_abs_pulse_enabled = abs_pulse_enabled;
    engine.m_abs_gain = abs_gain;

    engine.m_spin_enabled = spin_enabled;
    engine.m_spin_gain = spin_gain;
    engine.m_slide_texture_enabled = slide_enabled;
    engine.m_slide_texture_gain = slide_gain;
    engine.m_slide_freq_scale = slide_freq;
    engine.m_road_texture_enabled = road_enabled;
    engine.m_road_texture_gain = road_gain;
    engine.m_invert_force = invert_force;
    engine.m_max_torque_ref = max_torque_ref;
    engine.m_abs_freq_hz = abs_freq;
    engine.m_lockup_freq_scale = lockup_freq_scale;
    engine.m_spin_freq_scale = spin_freq_scale;
    engine.m_bottoming_method = bottoming_method;
    engine.m_scrub_drag_gain = scrub_drag_gain;
    engine.m_rear_align_effect = rear_align_effect;
    engine.m_sop_yaw_gain = sop_yaw_gain;
    engine.m_gyro_gain = gyro_gain;
    engine.m_steering_shaft_gain = steering_shaft_gain;
    engine.m_base_force_mode = base_force_mode;
    engine.m_flatspot_suppression = flatspot_suppression;
    engine.m_notch_q = notch_q;
    engine.m_flatspot_strength = flatspot_strength;
    engine.m_static_notch_enabled = static_notch_enabled;
    engine.m_static_notch_freq = static_notch_freq;
    engine.m_static_notch_width = static_notch_width;
    engine.m_yaw_kick_threshold = yaw_kick_threshold;
    engine.m_speed_gate_lower = speed_gate_lower;
    engine.m_speed_gate_upper = speed_gate_upper;
    engine.m_road_fallback_scale = road_fallback_scale;
    engine.m_understeer_affects_sop = understeer_affects_sop;
    engine.m_optimal_slip_angle = optimal_slip_angle;
    engine.m_optimal_slip_ratio = optimal_slip_ratio;
    engine.m_steering_shaft_smoothing = steering_shaft_smoothing;
    engine.m_gyro_smoothing = gyro_smoothing;
    engine.m_yaw_accel_smoothing = yaw_smoothing;
    engine.m_chassis_inertia_smoothing = chassis_smoothing;
  }

  // NEW: Capture current engine state into this preset
  void UpdateFromEngine(const FFBEngine &engine) {
    gain = engine.m_gain;
    understeer = engine.m_understeer_effect;
    sop = engine.m_sop_effect;
    sop_scale = engine.m_sop_scale;
    sop_smoothing = engine.m_sop_smoothing_factor;
    slip_smoothing = engine.m_slip_angle_smoothing;
    min_force = engine.m_min_force;
    oversteer_boost = engine.m_oversteer_boost;
    lockup_enabled = engine.m_lockup_enabled;
    lockup_gain = engine.m_lockup_gain;
    lockup_start_pct = engine.m_lockup_start_pct;
    lockup_full_pct = engine.m_lockup_full_pct;
    lockup_rear_boost = engine.m_lockup_rear_boost;
    lockup_gamma = engine.m_lockup_gamma;
    lockup_prediction_sens = engine.m_lockup_prediction_sens;
    lockup_bump_reject = engine.m_lockup_bump_reject;
    brake_load_cap = engine.m_brake_load_cap;
    texture_load_cap = engine.m_texture_load_cap; // NEW v0.6.25
    abs_pulse_enabled = engine.m_abs_pulse_enabled;
    abs_gain = engine.m_abs_gain;

    spin_enabled = engine.m_spin_enabled;
    spin_gain = engine.m_spin_gain;
    slide_enabled = engine.m_slide_texture_enabled;
    slide_gain = engine.m_slide_texture_gain;
    slide_freq = engine.m_slide_freq_scale;
    road_enabled = engine.m_road_texture_enabled;
    road_gain = engine.m_road_texture_gain;
    invert_force = engine.m_invert_force;
    max_torque_ref = engine.m_max_torque_ref;
    abs_freq = engine.m_abs_freq_hz;
    lockup_freq_scale = engine.m_lockup_freq_scale;
    spin_freq_scale = engine.m_spin_freq_scale;
    bottoming_method = engine.m_bottoming_method;
    scrub_drag_gain = engine.m_scrub_drag_gain;
    rear_align_effect = engine.m_rear_align_effect;
    sop_yaw_gain = engine.m_sop_yaw_gain;
    gyro_gain = engine.m_gyro_gain;
    steering_shaft_gain = engine.m_steering_shaft_gain;
    base_force_mode = engine.m_base_force_mode;
    flatspot_suppression = engine.m_flatspot_suppression;
    notch_q = engine.m_notch_q;
    flatspot_strength = engine.m_flatspot_strength;
    static_notch_enabled = engine.m_static_notch_enabled;
    static_notch_freq = engine.m_static_notch_freq;
    static_notch_width = engine.m_static_notch_width;
    yaw_kick_threshold = engine.m_yaw_kick_threshold;
    speed_gate_lower = engine.m_speed_gate_lower;
    speed_gate_upper = engine.m_speed_gate_upper;
    road_fallback_scale = engine.m_road_fallback_scale;
    understeer_affects_sop = engine.m_understeer_affects_sop;
    optimal_slip_angle = engine.m_optimal_slip_angle;
    optimal_slip_ratio = engine.m_optimal_slip_ratio;
    steering_shaft_smoothing = engine.m_steering_shaft_smoothing;
    gyro_smoothing = engine.m_gyro_smoothing;
    yaw_smoothing = engine.m_yaw_accel_smoothing;
    chassis_smoothing = engine.m_chassis_inertia_smoothing;
  }
};

struct ConfigData {
  std::string ini_version;
  bool ignore_vjoy_version_warning;
  bool enable_vjoy;
  bool output_ffb_to_vjoy;
  bool always_on_top;
  bool enable_auto_save;
  std::string last_device_guid;
  int win_pos_x;
  int win_pos_y;
  int win_w_small;
  int win_h_small;
  int win_w_large;
  int win_h_large;
  bool show_graphs;

  std::string selected_preset;
  std::vector<Preset> presets;

  Preset &GetPresetByName(const std::string &name) {
    for (auto &preset : presets) {
      if (preset.name == name) {
        return preset;
      }
    }
    throw std::runtime_error("Preset not found: " + name);
  }

  bool FromYAML(const YAML::Node &node) {
    try {
      ini_version = node["ini_version"].as<std::string>();
      ignore_vjoy_version_warning =
          node["ignore_vjoy_version_warning"].as<bool>();
      enable_vjoy = node["enable_vjoy"].as<bool>();
      output_ffb_to_vjoy = node["output_ffb_to_vjoy"].as<bool>();
      always_on_top = node["always_on_top"].as<bool>();
      enable_auto_save = node["enable_auto_save"].as<bool>(true);
      last_device_guid = node["last_device_guid"].as<std::string>();
      win_pos_x = node["win_pos_x"].as<int>();
      win_pos_y = node["win_pos_y"].as<int>();
      win_w_small = node["win_w_small"].as<int>();
      win_h_small = node["win_h_small"].as<int>();
      win_w_large = node["win_w_large"].as<int>();
      win_h_large = node["win_h_large"].as<int>();
      show_graphs = node["show_graphs"].as<bool>();
      selected_preset = node["selected_preset"].as<std::string>();

      presets.clear();
      for (const auto &preset_node : node["presets"]) {
        presets.emplace_back(Preset{
            .name = preset_node["name"].as<std::string>("Unnamed"),
            .is_builtin = preset_node["is_builtin"].as<bool>(false),
            .invert_force = preset_node["invert_force"].as<bool>(true),
            .gain = preset_node["gain"].as<float>(1.0f),
            .max_torque_ref = preset_node["max_torque_ref"].as<float>(1.0f),
            .min_force = preset_node["min_force"].as<float>(0.0f),
            .steering_shaft_gain =
                preset_node["steering_shaft_gain"].as<float>(1.0f),
            .steering_shaft_smoothing =
                preset_node["steering_shaft_smoothing"].as<float>(0.0f),
            .understeer = preset_node["understeer"].as<float>(1.0f),
            .base_force_mode = preset_node["base_force_mode"].as<int>(0),
            .flatspot_suppression =
                preset_node["flatspot_suppression"].as<bool>(false),
            .notch_q = preset_node["notch_q"].as<float>(0.0f),
            .flatspot_strength =
                preset_node["flatspot_strength"].as<float>(0.0f),
            .static_notch_enabled =
                preset_node["static_notch_enabled"].as<bool>(false),
            .static_notch_freq =
                preset_node["static_notch_freq"].as<float>(11.0f),
            .static_notch_width =
                preset_node["static_notch_width"].as<float>(2.0f),
            .oversteer_boost =
                preset_node["oversteer_boost"].as<float>(2.52101f),
            .sop = preset_node["sop"].as<float>(1.666f),
            .rear_align_effect =
                preset_node["rear_align_effect"].as<float>(0.666f),
            .sop_yaw_gain = preset_node["sop_yaw_gain"].as<float>(0.333f),
            .yaw_kick_threshold =
                preset_node["yaw_kick_threshold"].as<float>(0.0f),
            .yaw_smoothing = preset_node["yaw_smoothing"].as<float>(0.001f),
            .gyro_gain = preset_node["gyro_gain"].as<float>(0.0f),
            .gyro_smoothing = preset_node["gyro_smoothing"].as<float>(0.0f),
            .sop_smoothing = preset_node["sop_smoothing"].as<float>(1.0f),
            .slip_smoothing = preset_node["slip_smoothing"].as<float>(0.002f),
            .chassis_smoothing =
                preset_node["chassis_smoothing"].as<float>(0.0f),
            .optimal_slip_angle =
                preset_node["optimal_slip_angle"].as<float>(0.1f),
            .optimal_slip_ratio =
                preset_node["optimal_slip_ratio"].as<float>(0.12f),
            .lockup_enabled = preset_node["lockup_enabled"].as<bool>(true),
            .lockup_gain = preset_node["lockup_gain"].as<float>(0.37479f),
            .brake_load_cap = preset_node["brake_load_cap"].as<float>(2.0f),
            .lockup_freq_scale =
                preset_node["lockup_freq_scale"].as<float>(1.02f),
            .lockup_gamma = preset_node["lockup_gamma"].as<float>(0.1f),
            .lockup_start_pct = preset_node["lockup_start_pct"].as<float>(1.0f),
            .lockup_full_pct = preset_node["lockup_full_pct"].as<float>(5.0f),
            .lockup_prediction_sens =
                preset_node["lockup_prediction_sens"].as<float>(10.0f),
            .lockup_bump_reject =
                preset_node["lockup_bump_reject"].as<float>(0.1f),
            .lockup_rear_boost =
                preset_node["lockup_rear_boost"].as<float>(10.0f),
            .abs_pulse_enabled =
                preset_node["abs_pulse_enabled"].as<bool>(false),
            .abs_gain = preset_node["abs_gain"].as<float>(2.0f),
            .abs_freq = preset_node["abs_freq"].as<float>(25.5f),
            .texture_load_cap = preset_node["texture_load_cap"].as<float>(1.5f),
            .spin_enabled = preset_node["spin_enabled"].as<bool>(true),
            .spin_gain = preset_node["spin_gain"].as<float>(0.5f),
            .spin_freq_scale = preset_node["spin_freq_scale"].as<float>(1.0f),
            .scrub_drag_gain = preset_node["scrub_drag_gain"].as<float>(0.0f),
            .bottoming_method = preset_node["bottoming_method"].as<int>(0),
            .speed_gate_lower = preset_node["speed_gate_lower"].as<float>(1.0f),
            .speed_gate_upper = preset_node["speed_gate_upper"].as<float>(5.0f),
        });
      }
      return true;
    } catch (const YAML::Exception &e) {
      // Handle parsing errors
      return false;
    }
  }

  YAML::Node ToYAML() const {
    YAML::Node node;
    node["ini_version"] = ini_version;
    node["ignore_vjoy_version_warning"] = ignore_vjoy_version_warning;
    node["enable_vjoy"] = enable_vjoy;
    node["output_ffb_to_vjoy"] = output_ffb_to_vjoy;
    node["always_on_top"] = always_on_top;
    node["enable_auto_save"] = enable_auto_save;
    node["last_device_guid"] = last_device_guid;
    node["win_pos_x"] = win_pos_x;
    node["win_pos_y"] = win_pos_y;
    node["win_w_small"] = win_w_small;
    node["win_h_small"] = win_h_small;
    node["win_w_large"] = win_w_large;
    node["win_h_large"] = win_h_large;
    node["show_graphs"] = show_graphs;
    node["selected_preset"] = selected_preset;

    YAML::Node presets_node;
    for (const auto &preset : presets) {
      YAML::Node p_node;
      p_node["name"] = preset.name;
      p_node["is_builtin"] = preset.is_builtin;
      p_node["invert_force"] = preset.invert_force;
      p_node["gain"] = preset.gain;
      p_node["max_torque_ref"] = preset.max_torque_ref;
      p_node["min_force"] = preset.min_force;
      p_node["steering_shaft_gain"] = preset.steering_shaft_gain;
      p_node["steering_shaft_smoothing"] = preset.steering_shaft_smoothing;
      p_node["understeer"] = preset.understeer;
      p_node["base_force_mode"] = preset.base_force_mode;
      p_node["flatspot_suppression"] = preset.flatspot_suppression;
      p_node["notch_q"] = preset.notch_q;
      p_node["flatspot_strength"] = preset.flatspot_strength;
      p_node["static_notch_enabled"] = preset.static_notch_enabled;
      p_node["static_notch_freq"] = preset.static_notch_freq;
      p_node["static_notch_width"] = preset.static_notch_width;
      p_node["oversteer_boost"] = preset.oversteer_boost;
      p_node["sop"] = preset.sop;
      p_node["rear_align_effect"] = preset.rear_align_effect;
      p_node["sop_yaw_gain"] = preset.sop_yaw_gain;
      p_node["yaw_kick_threshold"] = preset.yaw_kick_threshold;
      p_node["yaw_smoothing"] = preset.yaw_smoothing;
      p_node["gyro_gain"] = preset.gyro_gain;
      p_node["gyro_smoothing"] = preset.gyro_smoothing;
      p_node["sop_smoothing"] = preset.sop_smoothing;
      p_node["sop_scale"] = preset.sop_scale;
      p_node["understeer_affects_sop"] = preset.understeer_affects_sop;
      p_node["slip_smoothing"] = preset.slip_smoothing;
      p_node["chassis_smoothing"] = preset.chassis_smoothing;
      p_node["optimal_slip_angle"] = preset.optimal_slip_angle;
      p_node["optimal_slip_ratio"] = preset.optimal_slip_ratio;
      p_node["lockup_enabled"] = preset.lockup_enabled;
      p_node["lockup_gain"] = preset.lockup_gain;
      p_node["brake_load_cap"] = preset.brake_load_cap;
      p_node["lockup_freq_scale"] = preset.lockup_freq_scale;
      p_node["lockup_gamma"] = preset.lockup_gamma;
      p_node["lockup_start_pct"] = preset.lockup_start_pct;
      p_node["lockup_full_pct"] = preset.lockup_full_pct;
      p_node["lockup_prediction_sens"] = preset.lockup_prediction_sens;
      p_node["lockup_bump_reject"] = preset.lockup_bump_reject;
      p_node["lockup_rear_boost"] = preset.lockup_rear_boost;
      p_node["abs_pulse_enabled"] = preset.abs_pulse_enabled;
      p_node["abs_gain"] = preset.abs_gain;
      p_node["abs_freq"] = preset.abs_freq;
      p_node["texture_load_cap"] = preset.texture_load_cap;
      p_node["slide_enabled"] = preset.slide_enabled;
      p_node["slide_gain"] = preset.slide_gain;
      p_node["slide_freq"] = preset.slide_freq;
      p_node["road_enabled"] = preset.road_enabled;
      p_node["road_gain"] = preset.road_gain;
      p_node["road_fallback_scale"] = preset.road_fallback_scale;
      p_node["spin_enabled"] = preset.spin_enabled;
      p_node["spin_gain"] = preset.spin_gain;
      p_node["spin_freq_scale"] = preset.spin_freq_scale;
      p_node["scrub_drag_gain"] = preset.scrub_drag_gain;
      p_node["bottoming_method"] = preset.bottoming_method;
      p_node["speed_gate_lower"] = preset.speed_gate_lower;
      p_node["speed_gate_upper"] = preset.speed_gate_upper;
      presets_node.push_back(p_node);
    }
    node["presets"] = presets_node;
    return node;
  }
};

class Config {
public:
  static Config &Get();

  void Save(const FFBEngine &engine, const std::string &filename = "");
  void Load(FFBEngine &engine, const std::string &filename = "");

  // Getters and Setters for Global App Settings
  bool GetIgnoreVJoyVersionWarning() const;
  void SetIgnoreVJoyVersionWarning(bool value);
  bool GetEnableVJoy() const;
  void SetEnableVJoy(bool value);
  bool GetOutputFFBToVJoy() const;
  void SetOutputFFBToVJoy(bool value);
  bool GetAlwaysOnTop() const;
  void SetAlwaysOnTop(bool value);
  bool GetAutoSave() const;
  void SetAutoSave(bool value);
  std::string GetLastDeviceGUID() const;
  void SetLastDeviceGUID(const std::string &guid);
  int GetWinPosX() const;
  void SetWinPosX(int x);
  int GetWinPosY() const;
  void SetWinPosY(int y);
  int GetWinWidthSmall() const;
  void SetWinWidthSmall(int w);
  int GetWinHeightSmall() const;
  void SetWinHeightSmall(int h);
  int GetWinWidthLarge() const;
  void SetWinWidthLarge(int w);
  int GetWinHeightLarge() const;
  void SetWinHeightLarge(int h);
  bool GetShowGraphs() const;
  void SetShowGraphs(bool show);
  std::string GetSelectedPresetName() const;
  void SetSelectedPresetName(const std::string &name);
  int GetSelectedPresetIndex() const;

  // Preset Management
  std::vector<Preset> &GetPresets();
  Preset *GetPreset(int index);
  void ApplyPreset(int index, FFBEngine &engine);
  int AddUserPreset(const std::string &name, const FFBEngine &engine);
  void SetPresetDirty(int index, bool dirty);
  bool IsPresetDirty(int index) const;
  void DeletePreset(int index);
  bool IsPresetBuiltin(int index) const;
  bool IsPresetBuiltin(const std::string &name) const;

private:
  std::string m_config_path_ = "config.yaml";
  ConfigData m_config_data_;
  bool m_preset_dirty_ = false;

  Config() = default;
  ~Config() = default;
  Config(const Config &) = delete;
  Config &operator=(const Config &) = delete;
};

#endif
