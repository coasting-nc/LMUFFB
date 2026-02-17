# Investigation Report - Issue #129: FFB Sample Rate Verification

## Problem Description
User reports a significantly reduced FFB sample rate (feels like 100Hz instead of the expected 400Hz from Le Mans Ultimate).

## Technical Analysis

### 1. FFB Loop Implementation (`src/main.cpp`)
The current `FFBThread` loop is structured as follows:
```cpp
while (g_running) {
    // 1. Copy Telemetry
    GameConnector::Get().CopyTelemetry(g_localData);

    // 2. Calculate Force (protected by mutex)
    {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        force = g_engine.calculate_force(...);
    }

    // 3. Update Hardware
    DirectInputFFB::Get().UpdateForce(force);

    // 4. Fixed Sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
}
```
**Issues found:**
- **Fixed Sleep**: A 2ms sleep results in a maximum theoretical frequency of 500Hz. However, this does not account for the time taken by steps 1, 2, and 3. If the combined processing time is 1ms, the total loop time becomes 3ms (~333Hz).
- **DirectInput Latency**: `DirectInputFFB::UpdateForce` calls `IDirectInputEffect::SetParameters`. In many drivers, this is a blocking call that can take anywhere from 1ms to 10ms depending on the hardware and driver efficiency. If it consistently takes ~8ms, the loop frequency drops to ~100Hz.
- **Mutex Contention**: The `g_engine_mutex` is shared with the GUI thread. The GUI thread (running at 60Hz) holds this lock while rendering the entire tuning window. If rendering takes 5ms, the FFB thread will be stalled for that duration every 16ms, causing jitter and dropped samples.

### 2. Telemetry Source (`src/GameConnector.cpp`)
The app relies on `mElapsedTime` changes in shared memory to detect new telemetry.
```cpp
double currentET = dest.telemetry.telemInfo[idx].mElapsedTime;
if (currentET != m_lastElapsedTime) {
    m_lastElapsedTime = currentET;
    m_lastUpdateLocalTime = std::chrono::steady_clock::now();
}
```
If the game itself is not updating shared memory at 400Hz, the app will process the same data multiple times or experience gaps.

### 3. Delta Time Handling (`src/FFBEngine.h`)
The engine uses `pPlayerTelemetry->mDeltaTime` for physics. If this is 0.01 (100Hz) instead of 0.0025 (400Hz), all filters and effects will operate at the lower frequency.

## Hypothesis
The reported "100Hz" feeling is likely caused by a combination of:
1. **DirectInput driver latency** slowing down the FFB loop.
2. **Fixed sleep** not leaving enough headroom for 400Hz.
3. **Mutex contention** with the GUI thread.
4. (Potential) Game-side configuration where LMU is not outputting 400Hz.

## Recommended Fixes
1. **Precise Timing**: Changed the FFB loop to use `std::this_thread::sleep_until` to maintain a steady 400Hz (2.5ms) period, compensating for processing time.
2. **Torque Source Selection**: Discovered that LMU provides a native 400Hz torque stream via `generic.FFBTorque` in shared memory, whereas the standard `mSteeringShaftTorque` in `TelemInfoV01` often ticks at 100Hz. Added a "Torque Source" setting to allow selecting the high-frequency stream.
3. **Monitoring & Diagnostics**:
    - Added a `RateMonitor` utility to track Hz for: FFB Loop, Telemetry Updates, Hardware Updates, and both Torque channels.
    - Added a "System Health" section in the GUI to display these rates in real-time with status colors.
    - Implemented console warnings if critical telemetry rates drop below target levels.

## Findings during Implementation
- **Shaft Torque (Legacy)**: Confirmed to be 100Hz in many sessions.
- **Direct Torque (LMU 1.2+)**: Confirmed to be 400Hz when the game is configured correctly.
- **Loop Jitter**: The fixed 2ms sleep was causing the FFB loop to run at variable frequencies depending on DirectInput latency. The new timing logic stabilizes this at 400Hz.
