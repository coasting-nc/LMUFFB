# Improve unsprung mass approximation in approximate_load and approximate_rear_load #345

**coasting-nc** opened on Mar 13, 2026

## Description

In approximate_load and approximate_rear_load currently we use a fixed value of 300.0 to approximate unsprung mass.

We should consider the following:
* car class specific unsprung mass values.
* different unsprung mass values for front and rear axle.
* any other significant improvement to the approximation.
* Improve the diagnostics of the log analyser about this.

The message below gives the details of what should be implemented (taken from docs\dev_docs\investigations\improve tire load estimation.md).

---

## Message 2: Implementation Details

### 1. C++ Implementation Improvements

First, we need to store the parsed vehicle class in the `FFBEngine` so we don't have to parse it every frame, and update the approximation functions to use motion ratios.

**File: `src/FFBEngine.h`**
Add the `m_current_vclass` member to the `FFBEngine` class:
```cpp
    // Context for Logging (v0.7.x)
    char m_vehicle_name[STR_BUF_64] = "Unknown";
    char m_track_name[STR_BUF_64] = "Unknown";
    std::string m_current_class_name = "";
    ParsedVehicleClass m_current_vclass = ParsedVehicleClass::UNKNOWN; // NEW: Store parsed class
```

**File: `src/GripLoadEstimation.cpp`**
Update `InitializeLoadReference` to cache the parsed class, and rewrite the approximation functions:
```cpp
// Initialize the load reference based on vehicle class and name seeding
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    // v0.7.109: Perform a full normalization reset on car change
    ResetNormalization();

    // Cache the parsed class for use in approximation functions
    m_current_vclass = ParseVehicleClass(className, vehicleName);
    ParsedVehicleClass vclass = m_current_vclass;

    // ... (rest of the function remains unchanged)
```

Replace the `approximate_load` and `approximate_rear_load` functions:
```cpp
// Helper: Approximate Load (v0.4.5 / Improved v0.7.171)
double FFBEngine::approximate_load(const TelemWheelV01& w) {
    double motion_ratio = 0.55;
    double unsprung_weight = 450.0; // ~45kg

    switch (m_current_vclass) {
        case ParsedVehicleClass::HYPERCAR:
        case ParsedVehicleClass::LMP2_UNRESTRICTED:
        case ParsedVehicleClass::LMP2_RESTRICTED:
        case ParsedVehicleClass::LMP2_UNSPECIFIED:
        case ParsedVehicleClass::LMP3:
            motion_ratio = 0.50;
            unsprung_weight = 400.0;
            break;
        case ParsedVehicleClass::GTE:
        case ParsedVehicleClass::GT3:
            motion_ratio = 0.65;
            unsprung_weight = 500.0;
            break;
        default:
            break;
    }

    return (std::max)(0.0, (w.mSuspForce * motion_ratio) + unsprung_weight);
}

double FFBEngine::approximate_rear_load(const TelemWheelV01& w) {
    double motion_ratio = 0.55;
    double unsprung_weight = 500.0; // ~50kg

    switch (m_current_vclass) {
        case ParsedVehicleClass::HYPERCAR:
        case ParsedVehicleClass::LMP2_UNRESTRICTED:
        case ParsedVehicleClass::LMP2_RESTRICTED:
        case ParsedVehicleClass::LMP2_UNSPECIFIED:
        case ParsedVehicleClass::LMP3:
            motion_ratio = 0.50;
            unsprung_weight = 450.0;
            break;
        case ParsedVehicleClass::GTE:
        case ParsedVehicleClass::GT3:
            motion_ratio = 0.65;
            unsprung_weight = 550.0;
            break;
        default:
            break;
    }

    return (std::max)(0.0, (w.mSuspForce * motion_ratio) + unsprung_weight);
}
```

**File: `src/VehicleUtils.cpp`**
Add "CADILLAC" to the Hypercar keyword list.

### 2. Python Log Analyzer Improvements

Add a statistical breakdown of the Load Estimation accuracy to the CLI output and the text reports.

[... Detailed Python changes for analyzers, cli, reports, and plots ...]
