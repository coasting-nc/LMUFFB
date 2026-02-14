# Implementation Plan - Dynamic Weight & Grip Smoothing

## 1. Context
This plan implements signal processing filters for the "Dynamic Weight" and "Load-Weighted Grip" features introduced in recent versions. Based on the physics analysis report, raw telemetry data for tire load and grip is too noisy (50Hz+ road noise) to be used directly for Force Feedback.

**Goal:**
1.  **Dynamic Weight:** Implement a standard Low Pass Filter (LPF) to simulate chassis pitch inertia/damping, filtering out road noise while retaining weight transfer feel.
2.  **Load-Weighted Grip:** Implement an **Adaptive Non-Linear Filter** to smooth out steady-state graininess while maintaining zero-latency response during rapid grip loss events (snap understeer).

**Reference Documents:**
*   Advice Report: `docs\dev_docs\reports\signal processing for Dynamic Weight & Load-Weighted Grip.md`

## 2. Codebase Analysis

### 2.1 Architecture Overview
*   **Physics Engine (`src/FFBEngine.h`):**
    *   `calculate_force`: Currently calculates Dynamic Weight using raw `ctx.avg_load`. Needs LPF logic.
    *   `calculate_grip`: Currently calculates grip using raw weighted average. Needs Adaptive Filter logic.
    *   **State Management:** New member variables are needed to store the previous frame's smoothed values (filter state).
*   **Configuration (`src/Config.h`, `src/Config.cpp`):**
    *   New parameters are required for smoothing time constants (`tau`) and sensitivity.
*   **UI (`src/GuiLayer_Common.cpp`):**
    *   New sliders needed to allow users to tune the smoothing feel.

### 2.2 Impacted Functionalities
*   **Dynamic Weight:** The steering weight modulation will become slower and smoother, simulating suspension dampers.
*   **Grip Estimation:** The `ctx.avg_grip` value will be less jittery on straights/steady corners but will still drop instantly during slides.

## 3. FFB Effect Impact Analysis

| Effect | Technical Impact | User Perspective |
| :--- | :--- | :--- |
| **Dynamic Weight** | **New Filter.** Raw load ratio is passed through an LPF ($\tau \approx 150ms$). | **Heavier, Less Buzzy.** Steering weight won't vibrate over curbs/texture but will still lighten under acceleration and heavy braking. |
| **Understeer (Grip)** | **New Adaptive Filter.** Raw grip is smoothed ($\tau \approx 50ms$) when stable, but smoothing drops ($\tau \approx 5ms$) when changing fast. | **Cleaner Feel.** Removes the "grainy" feeling of raw tire data. "Snap" oversteer/understeer cues remain instant. |

## 4. Proposed Changes

### 4.1 File: `src/FFBEngine.h`

**A. Add Helper Function**
Implement `apply_adaptive_smoothing` as a private helper.
```cpp
double apply_adaptive_smoothing(double input, double& prev_out, double dt, 
                                double slow_tau, double fast_tau, double sensitivity);
```

**B. Add State Variables**
```cpp
private:
    // Smoothing State
    double m_dynamic_weight_smoothed = 1.0;
    double m_grip_smoothed_state = 1.0;
```

**C. Add Configuration Variables**
```cpp
public:
    // Smoothing Settings
    float m_dynamic_weight_smoothing; // Default 0.15s
    float m_grip_smoothing_steady;    // Default 0.05s
    float m_grip_smoothing_fast;      // Default 0.005s
    float m_grip_smoothing_sensitivity; // Default 0.1
```

**D. Update `calculate_force` (Dynamic Weight)**
Apply standard LPF to `dynamic_weight_factor` before using it.

**E. Update `calculate_grip`**
Apply `apply_adaptive_smoothing` to the final `result.value`.

### 4.2 File: `src/Config.h`

**A. Update `Preset` Struct**
Add new parameters.
```cpp
struct Preset {
    // ...
    float dynamic_weight_smoothing = 0.15f;
    float grip_smoothing_steady = 0.05f;
    float grip_smoothing_fast = 0.005f;
    float grip_smoothing_sensitivity = 0.1f;
    // ...
};
```

**B. Parameter Synchronization Checklist**
For each new parameter (`dynamic_weight_smoothing`, `grip_smoothing_steady`, `grip_smoothing_fast`, `grip_smoothing_sensitivity`):
*   [ ] Add to `Preset` struct in `src/Config.h`.
*   [ ] Add to `Preset::Apply()` in `src/Config.h`.
*   [ ] Add to `Preset::UpdateFromEngine()` in `src/Config.h`.
*   [ ] Add to `Preset::Validate()` in `src/Config.h`.
*   [ ] Add to `Preset::Equals()` in `src/Config.h`.

### 4.3 File: `src/Config.cpp`

**A. Update Persistence**
*   [ ] Add to `Config::ParsePresetLine` (Load from INI).
*   [ ] Add to `Config::WritePresetFields` (Save to INI).
*   [ ] Add to `Config::Save` (Global config).
*   [ ] Add to `Config::Load` (Global config).

### 4.4 File: `src/GuiLayer_Common.cpp`

**A. Add UI Controls**
In `DrawTuningWindow`:
*   Under "Front Axle (Understeer)":
    *   Add slider for `Dynamic Weight Smoothing` (0.0s - 0.5s).
*   Under "Grip & Slip Angle Estimation":
    *   Add slider for `Grip Smoothing` (Controls `m_grip_smoothing_steady`, range 0.0s - 0.1s).
    *   (Optional) Add advanced tree for Fast Tau and Sensitivity if needed, otherwise keep hidden/default.

### 4.5 File: `VERSION` & `src/Version.h`
*   Increment version (e.g., `0.7.46` -> `0.7.47`).

## 5. Test Plan (TDD)

**New Test File:** `tests/test_ffb_smoothing.cpp`

### Test 1: `test_adaptive_smoothing_logic`
*   **Goal:** Verify the adaptive filter behaves correctly (slow smoothing on steady signal, fast on transient).
*   **Setup:**
    *   Initialize `prev_out = 0.0`.
    *   `slow_tau = 0.1`, `fast_tau = 0.0`.
    *   `sensitivity = 1.0`.
*   **Action 1 (Steady):** Input `0.1` (Delta 0.1 < Sensitivity).
    *   **Assert:** Output should move slowly towards 0.1 (High Alpha).
*   **Action 2 (Transient):** Input `10.0` (Delta 10.0 >> Sensitivity).
    *   **Assert:** Output should jump almost instantly to 10.0 (Low Alpha/Fast Tau).

### Test 2: `test_dynamic_weight_lpf`
*   **Goal:** Verify Dynamic Weight uses the configured smoothing.
*   **Setup:**
    *   Configure `dynamic_weight_smoothing = 1.0` (Very slow).
    *   Simulate Load Ratio change from 1.0 to 2.0.
*   **Action:** Run `calculate_force` for 1 frame (dt=0.01).
*   **Assert:** `dynamic_weight_factor` should barely move from 1.0.

### Test 3: `test_grip_smoothing_integration`
*   **Goal:** Verify `calculate_grip` applies smoothing to the result.
*   **Setup:**
    *   Configure `grip_smoothing_steady = 1.0` (Very slow).
    *   Input raw grip change 1.0 -> 0.5.
*   **Action:** Run `calculate_grip`.
*   **Assert:** Resulting grip value should be close to 1.0 (smoothed), not 0.5.

## 6. Deliverables

*   [ ] **Code:** Updated `src/FFBEngine.h` (Logic & State).
*   [ ] **Code:** Updated `src/Config.h` & `src/Config.cpp` (Settings).
*   [ ] **Code:** Updated `src/GuiLayer_Common.cpp` (UI).
*   [ ] **Tests:** New `tests/test_ffb_smoothing.cpp`.
*   [ ] **Implementation Notes:** Update plan with any tuning values found during testing.
 