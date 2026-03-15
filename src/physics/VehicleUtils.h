#ifndef VEHICLE_UTILS_H
#define VEHICLE_UTILS_H

#include <string>

enum class ParsedVehicleClass {
    UNKNOWN = 0,
    HYPERCAR,
    LMP2_UNRESTRICTED, // 8500N (ELMS/Unrestricted)
    LMP2_RESTRICTED,   // 7500N (WEC/Restricted)
    LMP2_UNSPECIFIED,  // 8000N (Generic Fallback)
    LMP3,              // 5800N
    GTE,               // 5500N
    LMGT3              // 5000N (LMU Specific)
};

// Returns a ParsedVehicleClass enum for internal logic and categorization
ParsedVehicleClass ParseVehicleClass(const char* className, const char* vehicleName);

// Lookup table: Map ParsedVehicleClass to Seed Load (Newtons)
double GetDefaultLoadForClass(ParsedVehicleClass vclass);

// Helper: String representation of parsed class for logging and UI
const char* VehicleClassToString(ParsedVehicleClass vclass);

// Helper: Parse vehicle brand from strings (v0.7.132)
const char* ParseVehicleBrand(const char* className, const char* vehicleName);

// Lookup table: Map ParsedVehicleClass to Motion Ratio (Wheel vs Pushrod)
double GetMotionRatioForClass(ParsedVehicleClass vclass);

// Lookup table: Map ParsedVehicleClass to Unsprung Weight (Newtons)
double GetUnsprungWeightForClass(ParsedVehicleClass vclass, bool is_rear);

#endif // VEHICLE_UTILS_H
