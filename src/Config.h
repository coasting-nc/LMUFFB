#ifndef CONFIG_H
#define CONFIG_H

#include "FFBEngine.h"
#include "PresetRegistry.h"
#include "Version.h"
#include <string>
#include <vector>
#include <chrono>

class Config {
public:
    static std::string m_config_path; // Default: "config.ini"
    static void Save(const FFBEngine& engine, const std::string& filename = "");
    static void Load(FFBEngine& engine, const std::string& filename = "");
    
    // NEW: Persist selected device
    static std::string m_last_device_guid;

    // Global App Settings (not part of FFB Physics)
    static bool m_ignore_vjoy_version_warning;
    static bool m_enable_vjoy;        // Acquire vJoy device (Driver Enabled)
    static bool m_output_ffb_to_vjoy; // Output FFB signal to vJoy Axis X (Monitor)
    static bool m_always_on_top;      // NEW: Keep window on top
    static bool m_auto_start_logging; // NEW: Auto-start logging
    static std::string m_log_path;    // NEW: Path to save logs

    // Window Geometry Persistence (v0.5.5)
    static int win_pos_x, win_pos_y;
    static int win_w_small, win_h_small; // Dimensions for Config Only
    static int win_w_large, win_h_large; // Dimensions for Config + Graphs
    static bool show_graphs;             // Remember if graphs were open
};

inline FFBEngine::FFBEngine() {
    last_log_time = std::chrono::steady_clock::now();
    Preset::ApplyDefaultsToEngine(*this);
}

#endif
