#ifndef CONFIG_H
#define CONFIG_H

#include "../FFBEngine.h"
#include <string>

class Config {
public:
    static void Save(const FFBEngine& engine, const std::string& filename = "config.ini");
    static void Load(FFBEngine& engine, const std::string& filename = "config.ini");

    // Global App Settings (not part of FFB Physics)
    static bool m_ignore_vjoy_version_warning;
    static bool m_enable_vjoy;        // Acquire vJoy device (Driver Enabled)
    static bool m_output_ffb_to_vjoy; // Output FFB signal to vJoy Axis X (Monitor)
};

#endif
