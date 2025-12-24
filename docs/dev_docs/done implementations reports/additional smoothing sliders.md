# Technical Specification: Advanced Signal Processing & Smoothing Exposure

**Target Version:** v0.5.0 (Beta)
**Date:** December 24, 2025
**Priority:** Medium (Advanced Tuning Features)

---

## 1. Executive Summary  & Context

While the default smoothing values in LMUFFB are tuned for a "Safe Baseline," advanced users with specific hardware (High-End Direct Drive) or specific car preferences (Hypercars vs. Historics) require granular control over the signal processing chain.

We are exposing three critical internal time constants to allow advanced tuning of FFB latency and feel. Instead of grouping them in a separate menu, they will be placed **contextually** next to the effects they modify.
Currently, three critical time constants are hardcoded or hidden:
1.  **Yaw Acceleration Smoothing:** Determines how fast the "Kick" warns of a slide.
2.  **Chassis Inertia (Kinematic):** Determines how fast the "Weight" transfers in the calculated physics model.
3.  **Gyro Damping Smoothing:** Determines the "texture" of the stabilizing resistance.

Here is where they are placed:
1.  **Yaw Acceleration Smoothing:** Placed in **Rear Axle**, under "Yaw Kick".
2.  **Gyro Damping Smoothing:** Placed in **Rear Axle**, under "Gyro Damping".
3.  **Chassis Inertia (Load):** Placed in **Grip & Slip Estimation**, as it controls the physics reconstruction model.

By exposing these, we transform LMUFFB from a "Black Box" into a professional tuning tool, allowing users to match the FFB latency to their specific hardware capabilities and the physical properties of the car they are driving.
---

## 2. Detailed Analysis of Settings

### A. Yaw Acceleration Smoothing (The "Kick" Response)
**Internal Variable:** `m_yaw_accel_smoothing` (Currently `const double tau_yaw = 0.0225`)
**Unit:** Seconds (Time Constant $\tau$).
**Function:** Filters the `mLocalRotAccel.y` derivative. Raw acceleration is extremely noisy; this filter extracts the "Trend" (The Slide) from the "Noise" (Vibration).

#### Tuning Guide & Values

| Value | Latency | Feel | Best For |
| :--- | :--- | :--- | :--- |
| **0.000 s** | **0 ms** | **Raw / Harsh.** Instant reaction, but prone to "spiking" and feedback loops with texture effects. | Debugging only. |
| **0.005 s** | **5 ms** | **Sharp.** The "Kick" hits instantly. Requires a very clean signal. | **High-End DD** (Simucube/VRS). |
| **0.010 s** | **10 ms** | **Fast (New Default).** Good balance. Filters out >16Hz noise (Slide Rumble) but passes the slide cue. | **Standard DD** (Fanatec/Moza). |
| **0.025 s** | **25 ms** | **Soft.** The "Kick" becomes a "Push." Easier to manage but slightly delayed. | **Belt/Gear** (T300/G29). |

---

### B. Chassis Inertia (Kinematic Load Transfer)

**Internal Variable:** `m_chassis_inertia_smoothing` (Currently `double chassis_tau = 0.035`)
**Unit:** Seconds (Time Constant $\tau$).
**Function:** Used **ONLY** when `mTireLoad` is blocked (Encrypted Content). It smooths the raw G-force inputs to simulate the physical time it takes for the car's suspension to compress and roll. It effectively acts as a **"Suspension Stiffness"** slider for the calculated load.

#### Tuning Guide & Values

| Value | Latency | Physics Simulation | Best For |
| :--- | :--- | :--- | :--- |
| **0.005 s** | **5 ms** | **Go-Kart / Rigid.** Weight transfer is digital/instant. Textures spike immediately on turn-in. | **Karting / Formula Cars.** |
| **0.025 s** | **25 ms** | **Stiff Race Car (New Default).** Fast, responsive weight transfer. | **Hypercars / LMP2 / GTE.** |
| **0.050 s** | **50 ms** | **Soft Suspension.** The car "heaves" and rolls into the corner. Textures swell up gradually. | **Historic Cars / Road Cars.** |
| **0.100 s** | **100 ms** | **Boat.** Very lazy weight transfer. | **Trucks / Soft Road Cars.** |

---

### C. Gyroscopic Damping Smoothing

**Internal Variable:** `m_gyro_smoothing` (Currently loaded from config but hidden).
**Unit:** Seconds (Time Constant $\tau$).
**Function:** Filters the `Steering Velocity` (derivative of position). Raw encoders have quantization noise ("steps"). If you damp based on raw velocity, the wheel feels "sandy" or "gritty." This filter turns that grit into a fluid viscosity.

#### Tuning Guide & Values

| Value | Latency | Feel | Best For |
| :--- | :--- | :--- | :--- |
| **0.002 s** | **2 ms** | **Raw.** You feel every micro-step of the encoder. Can feel like sand in the bearings. | **High-Res Encoders** (>20-bit). |
| **0.010 s** | **10 ms** | **Clean (Default).** Removes digital noise. Damping feels like hydraulic fluid. | **Most Direct Drive Wheels.** |
| **0.030 s** | **30 ms** | **Smooth.** Hides the "cogging" or "teeth" of mechanical wheels. | **Belt / Gear Driven** (T300/G29). |
---

## 3. Implementation Plan

### Phase 1: Core Engine (`FFBEngine.h`)

We need to promote local constants to member variables and update the defaults.

**1. Add Member Variables:**
```cpp
class FFBEngine {
public:
    // ... existing ...
    
    // NEW: Advanced Smoothing Parameters
    float m_yaw_accel_smoothing = 0.010f;      // Default 10ms (Was 22.5ms)
    float m_chassis_inertia_smoothing = 0.025f; // Default 25ms (Was 35ms)
    // m_gyro_smoothing already exists, just ensure default is set
    
    // ...
```

**2. Update `calculate_force` Logic:**

*   **Yaw Kick Section:**
    ```cpp
    // Old: const double tau_yaw = 0.0225;
    // New:
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < 0.0001) tau_yaw = 0.0001; // Safety
    double alpha_yaw = dt / (tau_yaw + dt);
    ```

*   **Kinematic Load Section:**
    ```cpp
    // Old: double chassis_tau = 0.035;
    // New:
    double chassis_tau = (double)m_chassis_inertia_smoothing;
    if (chassis_tau < 0.0001) chassis_tau = 0.0001;
    double alpha_chassis = dt / (chassis_tau + dt);
    ```

*   **Gyro Section:**
    *   *Correction:* Currently `m_gyro_smoothing` is treated as a 0-1 factor in the code. We need to change it to be a **Time Constant (seconds)** to match the others.
    *   *Migration:* If `m_gyro_smoothing > 0.5` (Legacy Factor), convert it? Or just reset it.
    *   *New Logic:*
        ```cpp
        double tau_gyro = (double)m_gyro_smoothing;
        if (tau_gyro < 0.0001) tau_gyro = 0.0001;
        double alpha_gyro = dt / (tau_gyro + dt);
        ```

---

### Phase 2: Configuration (`src/Config.h` & `.cpp`)

**1. Update `Preset` Struct:**
Add `yaw_smoothing` and `chassis_smoothing`. (Gyro already exists).

**2. Update `Config::Save` / `Config::Load`:**
Persist the new float values.

**3. Update `LoadPresets`:**
*   **Default (T300):**
    *   Yaw: `0.010f`
    *   Chassis: `0.025f`
    *   Gyro: `0.020f` (Smoother for belt)
*   **Direct Drive (Hypothetical):**
    *   Yaw: `0.005f`.
    *   Chassis: `0.025f`.
    *   Gyro: `0.005f`.

---

### Phase 3: GUI Layer (`src/GuiLayer.cpp`)

We will distribute the sliders into the existing sections.

#### A. Rear Axle Section (Update)
Insert the smoothing sliders immediately after their respective gain sliders.

```cpp
// Inside "Rear Axle" Tree Node...

// 1. Yaw Kick Gain
FloatSetting("Yaw Kick", &engine.m_sop_yaw_gain, ...);

// -> Yaw Kick Smoothing (Indented or distinct)
int yaw_ms = (int)(engine.m_yaw_accel_smoothing * 1000.0f);
ImGui::Text("  Kick Response"); ImGui::SameLine();
ImGui::TextColored((yaw_ms > 15) ? Red : Green, "(%d ms)", yaw_ms);
char yaw_fmt[32]; snprintf(yaw_fmt, 32, "%.3fs", engine.m_yaw_accel_smoothing);
FloatSetting("##YawSmooth", &engine.m_yaw_accel_smoothing, 0.0f, 0.050f, yaw_fmt);
if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reaction time of the slide kick.\nLower = Faster.\nHigher = Less noise.");

// 2. Gyro Gain
FloatSetting("Gyro Damping", &engine.m_gyro_gain, ...);

// -> Gyro Smoothing
int gyro_ms = (int)(engine.m_gyro_smoothing * 1000.0f);
ImGui::Text("  Gyro Smooth"); ImGui::SameLine();
ImGui::TextColored((gyro_ms > 20) ? Red : Green, "(%d ms)", gyro_ms);
char gyro_fmt[32]; snprintf(gyro_fmt, 32, "%.3fs", engine.m_gyro_smoothing);
FloatSetting("##GyroSmooth", &engine.m_gyro_smoothing, 0.0f, 0.050f, gyro_fmt);
if (ImGui::IsItemHovered()) ImGui::SetTooltip("Filters steering velocity.\nIncrease if damping feels 'sandy' or 'grainy'.");
```

#### B. Grip & Slip Section (Update)
Add Chassis Inertia here.

```cpp
// Inside "Grip & Slip Estimation" Tree Node...

// ... Existing Slip Angle Smoothing ...

ImGui::Separator();

// Chassis Inertia
int chassis_ms = (int)(engine.m_chassis_inertia_smoothing * 1000.0f);
ImGui::Text("Chassis Inertia (Load)"); ImGui::SameLine();
ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "(%d ms)", chassis_ms); // Blue text for Info

char chassis_fmt[32]; snprintf(chassis_fmt, 32, "%.3fs", engine.m_chassis_inertia_smoothing);
FloatSetting("##ChassisSmooth", &engine.m_chassis_inertia_smoothing, 0.0f, 0.100f, chassis_fmt);
if (ImGui::IsItemHovered()) ImGui::SetTooltip("Simulates suspension settling time for Calculated Load.\n25ms = Stiff Race Car.\n50ms = Soft Road Car.");
```


---

## 4. Verification Strategy

1.  **Unit Test (`test_time_corrected_smoothing`):**
    *   Update the test to use the new member variables instead of hardcoded values.
    *   Verify that setting `m_yaw_accel_smoothing = 0.0` results in instant signal transmission (no lag).
2.  **Manual Test:**
    *   **Yaw Kick:** Set to 50ms. Drive. The kick should feel late and disconnected. Set to 5ms. It should feel sharp.
    *   **Chassis:** Set to 100ms. Turn in. Road texture should fade in slowly. Set to 0ms. Texture should spike instantly.

---
