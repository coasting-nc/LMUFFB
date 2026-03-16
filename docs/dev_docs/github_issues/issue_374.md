# Fix: reset data missing detection logic when changing cars #374

**Open**
coasting-nc opened on Mar 16, 2026

## Description

Fix: reset data missing detection logic when changing cars

### Question

Review how we use `m_missing_vert_deflection_frames` in the code. Do we reset it if we switch cars? That is, if I leave the session, then start a new session with another car, do I reset the frames counter, and the flag that tells me if I have already printed the warning (`m_warned_vert_deflection`)?

Verify if we correctly do such resetting for each value that we check if it is missing.

### Answer

You have made a fantastic catch.

I just reviewed the codebase, specifically `FFBEngine.cpp` and `FFBEngine.h`, and no, we do not currently reset those counters or warning flags when switching cars.

Here is exactly how it behaves right now, and why your observation just prevented a major bug:

#### How it currently works:

1. **The Counters (`m_missing_vert_deflection_frames`, etc.):**
   These do self-correct, but organically. If you switch from an encrypted car to an unencrypted car, the code hits the else block and decrements the counter by 1 every frame:
   `m_missing_vert_deflection_frames = (std::max)(0, m_missing_vert_deflection_frames - 1);`
   At 400Hz, it takes about 125ms for the counter to drain back to zero. So the math fallbacks recover automatically.
2. **The Warning Flags (`m_warned_vert_deflection`, etc.):**
   These are never reset. Once they are set to true, they stay true until you completely close and restart the lmuFFB app.

#### Why this is a problem:

If we had gone with the approach of disabling the GUI slider using `m_warned_vert_deflection`, the slider would become permanently locked the moment you drove an encrypted car, even if you subsequently switched to an unencrypted car! Furthermore, the log file only prints the missing telemetry warning once per app launch, rather than once per car, which makes debugging logs confusing.

### The Fix

We need to explicitly reset all of these counters and flags whenever a new car is loaded.

The perfect place to do this is inside `FFBEngine::InitializeLoadReference()` in `src/physics/GripLoadEstimation.cpp`. This function is already triggered automatically by the FFBMetadataManager whenever a car change is detected.

Here is the code to add to `src/physics/GripLoadEstimation.cpp`:

```cpp
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    // v0.7.109: Perform a full normalization reset on car change
    ResetNormalization();

    // --- NEW FIX: Reset all missing telemetry counters and warning flags ---
    m_missing_load_frames = 0;
    m_missing_lat_force_front_frames = 0;
    m_missing_lat_force_rear_frames = 0;
    m_missing_susp_force_frames = 0;
    m_missing_susp_deflection_frames = 0;
    m_missing_vert_deflection_frames = 0;

    m_warned_load = false;
    m_warned_grip = false;
    m_warned_rear_grip = false;
    m_warned_lat_force_front = false;
    m_warned_lat_force_rear = false;
    m_warned_susp_force = false;
    m_warned_susp_deflection = false;
    m_warned_vert_deflection = false;
    // -----------------------------------------------------------------------

    ParsedVehicleClass vclass = ParseVehicleClass(className, vehicleName);
    // ... rest of the function remains the same
```

### Conclusion on the Kerb Feature

Because of this exact edge case (and the fact that `mSurfaceType` is universally available), the Hybrid Approach I outlined in the previous message is definitely the safest and most robust way forward.

By using `mSurfaceType == 5` as the primary trigger, and `mVerticalTireDeflection` as a secondary trigger, you don't need to mess with disabling GUI sliders at all. The slider will just gracefully do its best depending on the data the current car provides, and resetting the flags (as shown above) ensures the secondary trigger will immediately wake back up when switching to an unencrypted car.
