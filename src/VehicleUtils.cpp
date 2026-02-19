#include "VehicleUtils.h"
#include <algorithm>
#include <string>

// Helper: Parse car class from strings (v0.7.44 Refactor)
// Returns a ParsedVehicleClass enum for internal logic and categorization
ParsedVehicleClass ParseVehicleClass(const char* className, const char* vehicleName) {
    std::string cls = className ? className : "";
    std::string name = vehicleName ? vehicleName : "";

    // Normalize for case-insensitive matching
    std::transform(cls.begin(), cls.end(), cls.begin(), ::toupper);
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    // 1. Primary Identification via Class Name (Hierarchical)
    if (cls.find("HYPERCAR") != std::string::npos || cls.find("LMH") != std::string::npos || cls.find("LMDH") != std::string::npos) {
        return ParsedVehicleClass::HYPERCAR;
    }
    
    if (cls.find("LMP2") != std::string::npos) {
        if (cls.find("ELMS") != std::string::npos || name.find("DERESTRICTED") != std::string::npos) { return ParsedVehicleClass::LMP2_UNRESTRICTED; }
        volatile bool is_wec = (cls.find("WEC") != std::string::npos);
        if (is_wec) { return ParsedVehicleClass::LMP2_RESTRICTED; }
        return ParsedVehicleClass::LMP2_UNSPECIFIED;
    }

    if (cls.find("LMP3") != std::string::npos) return ParsedVehicleClass::LMP3;
    if (cls.find("GTE") != std::string::npos) return ParsedVehicleClass::GTE;
    if (cls.find("GT3") != std::string::npos || cls.find("LMGT3") != std::string::npos) return ParsedVehicleClass::GT3;

    // 2. Secondary Identification via Vehicle Name Keywords (Fallback)
    if (!name.empty()) {
        // Hypercars
        if (name.find("499P") != std::string::npos || name.find("GR010") != std::string::npos ||
            name.find("963") != std::string::npos || name.find("9X8") != std::string::npos ||
            name.find("V-SERIES.R") != std::string::npos || name.find("SCG 007") != std::string::npos ||
            name.find("GLICKENHAUS") != std::string::npos || name.find("VANWALL") != std::string::npos ||
            name.find("A424") != std::string::npos || name.find("SC63") != std::string::npos ||
            name.find("VALKYRIE") != std::string::npos || name.find("M HYBRID") != std::string::npos ||
            name.find("TIPO 6") != std::string::npos || name.find("680") != std::string::npos) {
            return ParsedVehicleClass::HYPERCAR;
        }
        
        // LMP2
        if (name.find("ORECA") != std::string::npos || name.find("07") != std::string::npos) {
            return ParsedVehicleClass::LMP2_UNSPECIFIED;
        }

        // LMP3
        if (name.find("LIGIER") != std::string::npos || name.find("GINETTA") != std::string::npos ||
            name.find("DUQUEINE") != std::string::npos || name.find("P320") != std::string::npos ||
            name.find("P325") != std::string::npos || name.find("G61") != std::string::npos ||
            name.find("D09") != std::string::npos) {
            return ParsedVehicleClass::LMP3;
        }

        // GTE
        if (name.find("RSR-19") != std::string::npos || name.find("488 GTE") != std::string::npos ||
            name.find("C8.R") != std::string::npos || name.find("VANTAGE AMR") != std::string::npos) {
            return ParsedVehicleClass::GTE;
        }

        // GT3
        if (name.find("LMGT3") != std::string::npos || name.find("296 GT3") != std::string::npos ||
            name.find("M4 GT3") != std::string::npos || name.find("Z06 GT3") != std::string::npos ||
            name.find("HURACAN") != std::string::npos || name.find("RC F") != std::string::npos ||
            name.find("720S") != std::string::npos || name.find("MUSTANG") != std::string::npos) {
            return ParsedVehicleClass::GT3;
        }
    }

    return ParsedVehicleClass::UNKNOWN;
}

// Lookup table: Map ParsedVehicleClass to Seed Load (Newtons)
double GetDefaultLoadForClass(ParsedVehicleClass vclass) {
    switch (vclass) {
        case ParsedVehicleClass::HYPERCAR:         return 9500.0;
        case ParsedVehicleClass::LMP2_UNRESTRICTED: return 8500.0;
        case ParsedVehicleClass::LMP2_RESTRICTED:   return 7500.0;
        case ParsedVehicleClass::LMP2_UNSPECIFIED:  return 8000.0;
        case ParsedVehicleClass::LMP3:             return 5800.0;
        case ParsedVehicleClass::GTE:              return 5500.0;
        case ParsedVehicleClass::GT3:              return 4800.0;
        default:                                   return 4500.0;
    }
}

// Helper: String representation of parsed class for logging and UI
const char* VehicleClassToString(ParsedVehicleClass vclass) {
    switch (vclass) {
        case ParsedVehicleClass::HYPERCAR:         return "Hypercar";
        case ParsedVehicleClass::LMP2_UNRESTRICTED: return "LMP2 Unrestricted";
        case ParsedVehicleClass::LMP2_RESTRICTED:   return "LMP2 Restricted";
        case ParsedVehicleClass::LMP2_UNSPECIFIED:  return "LMP2 Unspecified";
        case ParsedVehicleClass::LMP3:             return "LMP3";
        case ParsedVehicleClass::GTE:              return "GTE";
        case ParsedVehicleClass::GT3:              return "GT3";
        default:                                   return "Unknown";
    }
}
