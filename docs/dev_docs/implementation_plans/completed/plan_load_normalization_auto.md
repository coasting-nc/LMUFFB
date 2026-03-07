# Implementation Plan - Automatic Tire Load Normalization (Class-Based Seeding + Peak Hold)

## 1. Context
Currently, the FFB engine normalizes tire load by dividing the raw load (Newtons) by a hardcoded constant of `4000.0`. This value is tuned for GT3 cars (~4000N static + low aero).

**The Problem:**
High-downforce cars (Hypercars, LMP2) generate significantly higher peak loads (up to 10,000N).
*   **Result:** The normalized `load_factor` exceeds the texture load cap (default 1.5), causing road textures and effects to clip or feel "flat" at high speeds.
*   **User Experience:** Hypercars feel less detailed than GT3s, which is physically incorrect.

**The Solution:**
Implement an **Automatic Dynamic Reference** system.
1.  **Class-Based Seeding:** On session start (or car change), read the Vehicle Class string to set an immediate, correct baseline (e.g., 9500N for Hypercar). This eliminates "warm-up" lag.
2.  **Peak Hold Adaptation:** Continuously monitor load. If it exceeds the baseline, adapt immediately (Fast Attack). If it drops, decay slowly (Slow Decay).

**Reference Documents:**
*   `docs/dev_docs/investigations/slope_detection_feasibility.md` (General physics context)

## 2. Codebase Analysis

### 2.1 Architecture Overview
*   **`src/FFBEngine.h`:** Contains `calculate_force` where the normalization happens (`ctx.avg_load / 4000.0`).
*   **`src/main.cpp`:** The main loop retrieves telemetry and scoring data. `VehicleScoringInfoV01` contains the `mVehicleClass` string needed for seeding.
*   **`src/AsyncLogger.h`:** Logs telemetry. We should log the dynamic reference value for diagnostics.

### 2.2 Impacted Functionalities
*   **Road Texture:** Will now scale correctly for all car classes.
*   **Slide/Scrub Effects:** Will maintain consistent intensity relative to the car's capabilities.
*   **Lockup/Spin Effects:** Will use the normalized load factor, ensuring consistent vibration intensity across classes.

## 3. FFB Effect Impact Analysis

| Effect | Technical Impact | User Perspective |
| :--- | :--- | :--- |
| **Road Texture** | Input `load_factor` will now stay within 0.0-1.5 range for Hypercars instead of clipping at 2.0+. | **More Detail.** High-speed driving in prototypes will feel rich and textured, similar to GT3s, instead of numb/clipped. |
| **Slide/Scrub** | Normalized against the car's actual peak load. | **Consistent Feel.** A slide in a Hypercar will feel proportional to its grip limits, not artificially boosted by aero load. |
| **General** | No manual tuning required. | **"It Just Works."** Users don't need to adjust settings when switching car classes. |

## 4. Proposed Changes

### 4.1 Class-Based Seeding Table

| Class String Match | Approx Mass | Aero Level | Peak Front Load (Est) |
| :--- | :--- | :--- | :--- |
| **"GT3"** | ~1260 kg | Low | **4,800 N** |
| **"GTE"** | ~1245 kg | Medium-Low | **5,500 N** |
| **"LMP3"** | 950 kg | Medium | **5,800 N** |
| **"LMP2 (WEC)"** | 930 kg | High | **7,500 N** |
 **"LMP2 (ELMS)"** | 930 kg | Very High | **8,500 N** |
| **"Hypercar", "LMH", "LMDh"** | 1030 kg | Extreme | **9,500 N** |
| *Default / Unknown* | - | - | **4,500 N** |

### 4.2 File: `src/FFBEngine.h`

**A. Add Members**
```cpp
private:
    double m_auto_peak_load = 4500.0; // Dynamic reference
    std::string m_current_class_name = "";
```

**B. Add Helper Method**
```cpp
void InitializeLoadReference(const char* className) {
    std::string cls = className;
    // ... Implementation of the table above ...
    // Log the detection
}
```

**C. Update `calculate_force` Signature & Logic**
Change signature to accept class name (optional, to maintain test compatibility).
```cpp
double calculate_force(const TelemInfoV01* data, const char* vehicleClass = nullptr) {
    // ...
    
    // 1. Class Seeding
    if (vehicleClass && m_current_class_name != vehicleClass) {
        m_current_class_name = vehicleClass;
        InitializeLoadReference(vehicleClass);
    }

    // ... calculate ctx.avg_load ...

    // 2. Peak Hold Logic
    if (ctx.avg_load > m_auto_peak_load) {
        m_auto_peak_load = ctx.avg_load; // Fast Attack
    } else {
        m_auto_peak_load -= (100.0 * ctx.dt); // Slow Decay (~100N/s)
    }
    m_auto_peak_load = (std::max)(3000.0, m_auto_peak_load); // Safety Floor

    // 3. Normalize
    double raw_load_factor = ctx.avg_load / m_auto_peak_load;
    
    // ...
}
```

### 4.3 File: `src/main.cpp`

**A. Update Call Site**
Pass the vehicle class from scoring info.
```cpp
// Inside FFBThread loop
auto& scoring = g_localData.scoring.vehScoringInfo[idx];
// ...
force = g_engine.calculate_force(pPlayerTelemetry, scoring.mVehicleClass);
```

### 4.4 File: `src/AsyncLogger.h`

**A. Update LogFrame**
Add `float load_peak_ref;` to `LogFrame`.

**B. Update Logging**
Log `m_auto_peak_load` to verify the system is working.

### 4.5 File: `VERSION` & `src/Version.h`
*   Increment version (e.g., `0.7.42` -> `0.7.43`).

## 5. Test Plan (TDD)

**New Test File:** `tests/test_ffb_load_normalization.cpp`

### Test 1: `test_class_seeding`
*   **Goal:** Verify correct baseline initialization for different strings.
*   **Setup:** Call `calculate_force` with different class strings.
*   **Assertions:**
    *   "Hypercar" -> `m_auto_peak_load` approx 9500.
    *   "LMP2" -> approx 8000.
    *   "GT3" -> approx 4800.

### Test 2: `test_peak_hold_adaptation`
*   **Goal:** Verify "Fast Attack" behavior.
*   **Setup:**
    *   Initialize as GT3 (4800N).
    *   Feed telemetry with 6000N load.
*   **Assertion:** `m_auto_peak_load` increases to 6000N immediately.

### Test 3: `test_peak_hold_decay`
*   **Goal:** Verify "Slow Decay" behavior.
*   **Setup:**
    *   Set peak to 8000N.
    *   Feed telemetry with 4000N load for 1 second.
*   **Assertion:** `m_auto_peak_load` decreases slightly (approx 7900N), not instantly to 4000N.

## 6. Deliverables

*   [ ] **Code:** Updated `src/FFBEngine.h` (Logic).
*   [ ] **Code:** Updated `src/main.cpp` (Integration).
*   [ ] **Code:** Updated `src/AsyncLogger.h` (Diagnostics).
*   [ ] **Tests:** New `tests/test_ffb_load_normalization.cpp`.
*   [ ] **Implementation Notes:** Update plan with any tuning adjustments made to the decay rate.