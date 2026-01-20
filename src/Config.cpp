#include "Config.h"
#include "Version.h"
#include <algorithm>
#include <fstream>
#include <iostream>

Config &Config::Get() {
  static Config instance;
  return instance;
}

void Config::Save(const FFBEngine &engine, const std::string &filename) {
  std::string final_path = filename.empty() ? m_config_path_ : filename;
  std::ofstream o_file(final_path);
  if (!o_file.is_open()) {
    std::cout << "[Config] Failed to open config for saving: " << final_path
              << std::endl;
    return;
  }

  o_file << m_config_data_.ToYAML();
  o_file.close();
  m_preset_dirty_ = false;
  std::cout << "[Config] Saved config to " << final_path << std::endl;
}

void Config::Load(FFBEngine &engine, const std::string &filename) {
  std::string final_path = filename.empty() ? m_config_path_ : filename;
  m_config_data_ = ConfigData();
  m_config_data_.FromYAML(YAML::LoadFile(final_path));
  auto preset = m_config_data_.GetPresetByName(m_config_data_.selected_preset);
  preset.Apply(engine);

  // v0.5.7: Safety Validation - Prevent Division by Zero in Grip Calculation
  if (engine.m_optimal_slip_angle < 0.01f) {
    std::cerr << "[Config] Invalid optimal_slip_angle ("
              << engine.m_optimal_slip_angle << "), resetting to default 0.10"
              << std::endl;
    engine.m_optimal_slip_angle = 0.10f;
  }
  if (engine.m_optimal_slip_ratio < 0.01f) {
    std::cerr << "[Config] Invalid optimal_slip_ratio ("
              << engine.m_optimal_slip_ratio << "), resetting to default 0.12"
              << std::endl;
    engine.m_optimal_slip_ratio = 0.12f;
  }

  // v0.6.20: Safety Validation - Clamp Advanced Braking Parameters to Valid
  // Ranges (Expanded)
  if (engine.m_lockup_gamma < 0.1f || engine.m_lockup_gamma > 3.0f) {
    std::cerr << "[Config] Invalid lockup_gamma (" << engine.m_lockup_gamma
              << "), clamping to range [0.1, 3.0]" << std::endl;
    engine.m_lockup_gamma =
        (std::max)(0.1f, (std::min)(3.0f, engine.m_lockup_gamma));
  }
  if (engine.m_lockup_prediction_sens < 10.0f ||
      engine.m_lockup_prediction_sens > 100.0f) {
    std::cerr << "[Config] Invalid lockup_prediction_sens ("
              << engine.m_lockup_prediction_sens
              << "), clamping to range [10.0, 100.0]" << std::endl;
    engine.m_lockup_prediction_sens =
        (std::max)(10.0f, (std::min)(100.0f, engine.m_lockup_prediction_sens));
  }
  if (engine.m_lockup_bump_reject < 0.1f ||
      engine.m_lockup_bump_reject > 5.0f) {
    std::cerr << "[Config] Invalid lockup_bump_reject ("
              << engine.m_lockup_bump_reject
              << "), clamping to range [0.1, 5.0]" << std::endl;
    engine.m_lockup_bump_reject =
        (std::max)(0.1f, (std::min)(5.0f, engine.m_lockup_bump_reject));
  }
  if (engine.m_abs_gain < 0.0f || engine.m_abs_gain > 10.0f) {
    std::cerr << "[Config] Invalid abs_gain (" << engine.m_abs_gain
              << "), clamping to range [0.0, 10.0]" << std::endl;
    engine.m_abs_gain = (std::max)(0.0f, (std::min)(10.0f, engine.m_abs_gain));
  }
  // Legacy Migration: Convert 0-200 range to 0-2.0 range
  if (engine.m_understeer_effect > 2.0f) {
    float old_val = engine.m_understeer_effect;
    engine.m_understeer_effect = engine.m_understeer_effect / 100.0f;
    std::cout << "[Config] Migrated legacy understeer_effect: " << old_val
              << " -> " << engine.m_understeer_effect << std::endl;
  }
  // Clamp to new valid range [0.0, 2.0]
  if (engine.m_understeer_effect < 0.0f || engine.m_understeer_effect > 2.0f) {
    engine.m_understeer_effect =
        (std::max)(0.0f, (std::min)(2.0f, engine.m_understeer_effect));
  }
  if (engine.m_steering_shaft_gain < 0.0f ||
      engine.m_steering_shaft_gain > 2.0f) {
    engine.m_steering_shaft_gain =
        (std::max)(0.0f, (std::min)(2.0f, engine.m_steering_shaft_gain));
  }
  if (engine.m_lockup_gain < 0.0f || engine.m_lockup_gain > 3.0f) {
    engine.m_lockup_gain =
        (std::max)(0.0f, (std::min)(3.0f, engine.m_lockup_gain));
  }
  if (engine.m_brake_load_cap < 1.0f || engine.m_brake_load_cap > 10.0f) {
    engine.m_brake_load_cap =
        (std::max)(1.0f, (std::min)(10.0f, engine.m_brake_load_cap));
  }
  if (engine.m_lockup_rear_boost < 1.0f || engine.m_lockup_rear_boost > 10.0f) {
    engine.m_lockup_rear_boost =
        (std::max)(1.0f, (std::min)(10.0f, engine.m_lockup_rear_boost));
  }
  if (engine.m_oversteer_boost < 0.0f || engine.m_oversteer_boost > 4.0f) {
    engine.m_oversteer_boost =
        (std::max)(0.0f, (std::min)(4.0f, engine.m_oversteer_boost));
  }
  if (engine.m_sop_yaw_gain < 0.0f || engine.m_sop_yaw_gain > 1.0f) {
    engine.m_sop_yaw_gain =
        (std::max)(0.0f, (std::min)(1.0f, engine.m_sop_yaw_gain));
  }
  if (engine.m_slide_texture_gain < 0.0f ||
      engine.m_slide_texture_gain > 2.0f) {
    engine.m_slide_texture_gain =
        (std::max)(0.0f, (std::min)(2.0f, engine.m_slide_texture_gain));
  }
  if (engine.m_road_texture_gain < 0.0f || engine.m_road_texture_gain > 2.0f) {
    engine.m_road_texture_gain =
        (std::max)(0.0f, (std::min)(2.0f, engine.m_road_texture_gain));
  }
  if (engine.m_spin_gain < 0.0f || engine.m_spin_gain > 2.0f) {
    engine.m_spin_gain = (std::max)(0.0f, (std::min)(2.0f, engine.m_spin_gain));
  }
  if (engine.m_rear_align_effect < 0.0f || engine.m_rear_align_effect > 2.0f) {
    engine.m_rear_align_effect =
        (std::max)(0.0f, (std::min)(2.0f, engine.m_rear_align_effect));
  }
  if (engine.m_sop_effect < 0.0f || engine.m_sop_effect > 2.0f) {
    engine.m_sop_effect =
        (std::max)(0.0f, (std::min)(2.0f, engine.m_sop_effect));
  }
  std::cout << "[Config] Loaded from " << filename << std::endl;
}

// ============================================================================
// Global App Settings Getters and Setters
// ============================================================================

bool Config::GetIgnoreVJoyVersionWarning() const {
  return m_config_data_.ignore_vjoy_version_warning;
}

void Config::SetIgnoreVJoyVersionWarning(bool value) {
  m_config_data_.ignore_vjoy_version_warning = value;
}

bool Config::GetEnableVJoy() const { return m_config_data_.enable_vjoy; }

void Config::SetEnableVJoy(bool value) { m_config_data_.enable_vjoy = value; }

bool Config::GetOutputFFBToVJoy() const {
  return m_config_data_.output_ffb_to_vjoy;
}

void Config::SetOutputFFBToVJoy(bool value) {
  m_config_data_.output_ffb_to_vjoy = value;
}
bool Config::GetAlwaysOnTop() const { return m_config_data_.always_on_top; }

void Config::SetAlwaysOnTop(bool value) {
  m_config_data_.always_on_top = value;
}
bool Config::GetAutoSave() const { return m_config_data_.enable_auto_save; }
void Config::SetAutoSave(bool value) {
  m_config_data_.enable_auto_save = value;
}

std::string Config::GetLastDeviceGUID() const {
  return m_config_data_.last_device_guid;
}
void Config::SetLastDeviceGUID(const std::string &guid) {
  m_config_data_.last_device_guid = guid;
}

int Config::GetWinPosX() const { return m_config_data_.win_pos_x; }

void Config::SetWinPosX(int x) { m_config_data_.win_pos_x = x; }

int Config::GetWinPosY() const { return m_config_data_.win_pos_y; }
void Config::SetWinPosY(int y) { m_config_data_.win_pos_y = y; }

int Config::GetWinWidthSmall() const { return m_config_data_.win_w_small; }

void Config::SetWinWidthSmall(int w) { m_config_data_.win_w_small = w; }

int Config::GetWinHeightSmall() const { return m_config_data_.win_h_small; }

void Config::SetWinHeightSmall(int h) { m_config_data_.win_h_small = h; }
int Config::GetWinWidthLarge() const { return m_config_data_.win_w_large; }

void Config::SetWinWidthLarge(int w) { m_config_data_.win_w_large = w; }

int Config::GetWinHeightLarge() const { return m_config_data_.win_h_large; }
void Config::SetWinHeightLarge(int h) { m_config_data_.win_h_large = h; }

bool Config::GetShowGraphs() const { return m_config_data_.show_graphs; }

void Config::SetShowGraphs(bool show) { m_config_data_.show_graphs = show; }
std::string Config::GetSelectedPresetName() const {
  return m_config_data_.selected_preset;
}

void Config::SetSelectedPresetName(const std::string &name) {
  m_config_data_.selected_preset = name;
}

int Config::GetSelectedPresetIndex() const {
  ;
  for (size_t i = 0; i < m_config_data_.presets.size(); i++) {
    if (m_config_data_.presets[i].name == m_config_data_.selected_preset) {
      return static_cast<int>(i);
    }
  }
  return -1; // Not found
}

// ============================================================================
std::vector<Preset> &Config::GetPresets() { return m_config_data_.presets; }
Preset *Config::GetPreset(int index) {
  if (index >= 0 && index < m_config_data_.presets.size()) {
    return &m_config_data_.presets[index];
  }
  return nullptr;
}

void Config::ApplyPreset(int index, FFBEngine &engine) {
  if (index >= 0 && index < m_config_data_.presets.size()) {
    m_config_data_.presets[index].Apply(engine);
    SetSelectedPresetName(m_config_data_.presets[index].name);
    std::cout << "[Config] Applied preset: "
              << m_config_data_.presets[index].name << std::endl;
    Save(engine); // Integrated Auto-Save (v0.6.27)
  }
}

int Config::AddUserPreset(const std::string &name, const FFBEngine &engine) {
  // Check if name exists and overwrite, or add new
  bool found = false;
  for (auto &p : m_config_data_.presets) {
    if (p.name == name && !p.is_builtin) {
      p.UpdateFromEngine(engine);
      // Return index of updated preset
      for (size_t i = 0; i < m_config_data_.presets.size(); i++) {
        if (m_config_data_.presets[i].name == name) {
          return static_cast<int>(i);
        }
      }
    }
  }

  Preset p;
  p.name = name;
  p.UpdateFromEngine(engine);
  m_config_data_.presets.push_back(p);

  // Save immediately to persist
  Save(engine);

  return static_cast<int>(m_config_data_.presets.size() - 1);
}

void Config::SetPresetDirty(int index, bool dirty) {
  if (index >= 0 && index < m_config_data_.presets.size()) {
    m_preset_dirty_ = dirty;
  }
}

bool Config::IsPresetDirty(int index) const {
  if (index >= 0 && index < m_config_data_.presets.size()) {
    return m_preset_dirty_;
  }
  return false;
}

void Config::DeletePreset(int index) {
  if (index >= 0 && index < m_config_data_.presets.size()) {
    if (m_config_data_.presets[index].is_builtin) {
      std::cout << "[Config] Cannot delete built-in preset: "
                << m_config_data_.presets[index].name << std::endl;
      return;
    }
    std::cout << "[Config] Deleting preset: "
              << m_config_data_.presets[index].name << std::endl;
    m_config_data_.presets.erase(m_config_data_.presets.begin() + index);
  }
}

bool Config::IsPresetBuiltin(int index) const {
  if (index >= 0 && index < m_config_data_.presets.size()) {
    return m_config_data_.presets[index].is_builtin;
  }
  return false;
}

bool Config::IsPresetBuiltin(const std::string &name) const {
  for (const auto &preset : m_config_data_.presets) {
    if (preset.name == name) {
      return preset.is_builtin;
    }
  }
  return false;
}