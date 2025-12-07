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
};

#endif
