User question for a review:

Review this change that was implemented. Is it safe, or does it risk reintroducing spikes and pulls due to 100Hz raw acceleration signals?

// If upsampling is OFF and raw accel was provided, prefer raw accel ONLY IF 
// the derived acceleration logic is likely inactive (e.g. first frame or missing velocity).
// Otherwise, the derived 100Hz signals (now upsampled to 400Hz) are safer.
// RELIABILITY FIX: Only use raw accel if we haven't established a velocity baseline yet
// OR if the raw acceleration is clearly dominant (legacy test compatibility).
if (!upsampling_active) {
    if (std::abs(data->mLocalAccel.y) > 0.001 || std::abs(data->mLocalAccel.z) > 0.001) {
        m_working_info.mLocalAccel.y = data->mLocalAccel.y;
        m_working_info.mLocalAccel.z = data->mLocalAccel.z;
    }
}

----


Review:




This change is **NOT safe** for production and directly risks reintroducing the exact spikes and pulls that the "Derived Acceleration (Issue #278)" logic was created to fix. 

Here is a detailed breakdown of why this snippet is dangerous, what it breaks, and how to properly handle the problem it was trying to solve.

### 1. It Undermines the Fix for Issue #278
In `FFBEngine.cpp`, you specifically calculate `m_derived_accel_z_100hz` from `mLocalVel.z` because the raw `data->mLocalAccel.z` channel in the game engine is notoriously noisy and prone to massive, instantaneous physics spikes (especially over kerbs or during collisions).

By adding this block:
```cpp
if (!upsampling_active) {
    if (std::abs(data->mLocalAccel.y) > 0.001 || std::abs(data->mLocalAccel.z) > 0.001) {
        m_working_info.mLocalAccel.y = data->mLocalAccel.y;
        m_working_info.mLocalAccel.z = data->mLocalAccel.z;
    }
}
```
You are saying: *"If we are running at 100Hz (legacy mode/no upsampling), throw away the smooth derived acceleration and use the raw, spikey game acceleration instead."*

The threshold `> 0.001` is practically zero. The moment the car moves, this condition evaluates to `true`, completely bypassing your safe derived math.

### 2. The Symptoms It Will Reintroduce
If a user runs the app in a state where `upsampling_active == false` (e.g., legacy rFactor 2 mode, or if the 400Hz thread falls back to 100Hz), they will experience:
* **Violent Steering Pulls:** `mLocalAccel.z` drives the **Longitudinal Load** effect. A raw spike in Z-acceleration will cause the FFB multiplier to instantly jump, yanking the wheel out of the user's hands.
* **Harsh Rattling:** `mLocalAccel.y` is used as a fallback for **Road Texture** and **Bottoming**. Raw Y-acceleration spikes will translate directly into high-frequency mechanical "clacks" in the wheelbase.

### 3. The "Test Compatibility" Anti-Pattern
The comment mentions: `OR if the raw acceleration is clearly dominant (legacy test compatibility)`.

This indicates that this code was added because some older unit tests were failing. Those tests likely inject a fake `mLocalAccel.z` value to test the Longitudinal Load effect, but they *forgot* to inject a changing `mLocalVel.z` value. Because the velocity wasn't changing in the test, your new derived math calculated `0.0` acceleration, causing the test to fail.

**You should never compromise production physics to make a lazy unit test pass.** 

### How to Fix It Properly

**Option A: Fix the Unit Tests (Recommended)**
Remove that entire `if (!upsampling_active)` block from `FFBEngine.cpp`. 
Then, go into your unit tests and update them so that instead of just setting `data.mLocalAccel.z = 10.0;`, they simulate velocity over time:
```cpp
// In the test:
data.mLocalVel.z = previous_vel + (10.0 * dt); 
```
This ensures your tests actually validate the real production pipeline (the derivative math).

**Option B: Use a Compiler Directive (If you must hack it)**
If you absolutely cannot fix the unit tests right now and need them to pass, wrap the hack in a preprocessor macro so it is physically impossible for it to run in the release build given to users:

```cpp
#ifdef LMUFFB_UNIT_TEST
    // RELIABILITY FIX: Only use raw accel for legacy test compatibility
    // where tests inject raw accel but fail to simulate velocity.
    if (!upsampling_active) {
        if (std::abs(data->mLocalAccel.y) > 0.001 || std::abs(data->mLocalAccel.z) > 0.001) {
            m_working_info.mLocalAccel.y = data->mLocalAccel.y;
            m_working_info.mLocalAccel.z = data->mLocalAccel.z;
        }
    }
#endif
```

### Summary
Do not leave that code active in the production build. It creates a dangerous trap door where anyone running the app outside of the strict 400Hz LMU environment will be subjected to raw telemetry spikes, ruining the FFB experience and potentially causing hardware jolts.

---

[The user provided more information to perform the code review]

---




Thank you for providing the implementation plan. This context is incredibly helpful and confirms exactly what we suspected in the previous analysis.

In the **"Deviations from the Plan"** section, the author explicitly states:
> *"**Derived Accel Compatibility**: Modified derived acceleration logic to prefer raw inputs if they are non-zero and upsampling is inactive, providing compatibility for legacy tests that manually inject acceleration."*

This confirms that this code snippet is a **hack added specifically to avoid fixing broken unit tests**. 

Here is a contextualized, in-depth review of this specific change, its safety, and why it should be rejected.

### 1. Is it safe in production right now?
**Technically yes, but only by coincidence.** 
If you look at `src/core/main.cpp` (around line 160), the main FFB thread calls the engine like this:
```cpp
force_physics = g_engine.calculate_force(pPlayerTelemetry, scoring.mVehicleClass, scoring.mVehicleName, g_localData.generic.FFBTorque, full_allowed, 0.0025, scoring.mControl);
```
Because `0.0025` is explicitly passed as the `override_dt` argument, `upsampling_active` evaluates to `true`. Therefore, the `if (!upsampling_active)` block is **dead code in production**. It will never execute while the user is playing the game, so it will not immediately reintroduce the spikes.

### 2. Why it is still an Architectural Time Bomb
Even though it is currently dead code in production, leaving this in the codebase is highly dangerous for several reasons:

* **The Default Argument Trap:** In `FFBEngine.h`, the signature for this function is:
  ```cpp
  double calculate_force(..., double override_dt = -1.0, ...);
  ```
  If a future developer, modder, or a different integration of this codebase calls `calculate_force(data)` and relies on the default arguments, `upsampling_active` will evaluate to `false`. The trapdoor will instantly open, the raw acceleration spikes will bypass the safety filters, and the violent steering pulls will return.
* **Test-Induced Design Damage:** This is a well-known software engineering anti-pattern. Production code should **never** contain logic branches that exist solely to make outdated tests pass. The tests exist to verify the production physics, not the other way around. If the production physics changed to derive acceleration from velocity (to fix a major bug), the tests *must* be updated to reflect that new physical reality.

### 3. The Correct Way to Fix the Tests
The implementation plan notes that fixing the "Seeding Gate" broke 16 legacy unit tests, and the author systematically updated them. They should have done the exact same thing for the acceleration tests.

Instead of injecting raw acceleration into a static frame:
```cpp
// BAD LEGACY TEST
TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
data.mLocalAccel.z = 10.0; // Injecting raw accel
engine.calculate_force(&data);
```

The tests should be updated to simulate velocity over time, which naturally triggers the new, safe derived acceleration math:
```cpp
// GOOD UPDATED TEST
TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
for (int i = 0; i < 5; i++) {
    data.mLocalVel.z += (10.0 * data.mDeltaTime); // Simulate 10m/s^2 acceleration
    engine.calculate_force(&data);
}
```

### Conclusion & Recommendation
**Reject this specific deviation from the plan.** 

You should instruct the author to:
1. Remove the `if (!upsampling_active)` block entirely from `FFBEngine.cpp`.
2. Update the failing legacy unit tests to properly simulate changing `mLocalVel.y` and `mLocalVel.z` over multiple frames, rather than injecting static `mLocalAccel` values. 

The rest of the Implementation Plan (fixing the 4 state-contamination bugs, resetting the normalization, and clearing the slope buffers) is excellent and correctly addresses the root causes of the session transition bugs.