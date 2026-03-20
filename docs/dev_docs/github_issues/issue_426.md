# Pause/Menu Spikes when the wheel is off-center #426

**coasting-nc** opened this issue on Mar 19, 2026

## Description

Pause/Menu Spikes when the wheel is off-center

### Issue 1: The "Pause/Menu Spikes" when the wheel is off-center

**The User's Clue:** "As long as my wheel is 0 degrees, if i exit to menu i get no spikes. So something regarding axis degrees?"

**The Cause:**
When the user exits to the menu or pauses, lmuFFB correctly detects that the player is no longer in control and mutes the FFB (allowed = false).

However, look at how the safety slew (fade-out) is handled in `src/ffb/FFBSafetyMonitor.h`:
```cpp
static constexpr float SAFETY_SLEW_NORMAL = 1000.0f;
static constexpr float SAFETY_SLEW_RESTRICTED = 100.0f; // <--- THE CULPRIT
```

And in `FFBSafetyMonitor.cpp`:
```cpp
double slew_limit = restricted ? (double)SAFETY_SLEW_RESTRICTED : (double)SAFETY_SLEW_NORMAL;
double max_delta = slew_limit * dt; // 100.0 * 0.0025 = 0.25 per tick
```

Because the FFB signal is normalized between -1.0 and 1.0, a slew limit of 100.0 means the force drops from 100% to 0% in just 10 milliseconds.

If the wheel is turned (e.g., mid-corner) and under heavy tension, dropping the torque to zero in 10ms acts like a square wave. The physical motor snaps back violently, causing a loud "clack" or spike. If the wheel is at 0 degrees, there is no tension, so dropping to zero does nothing (which perfectly matches the user's report).

**The Fix:**
Lower the restricted slew rate so the force fades out smoothly over ~0.5 seconds when pausing or entering the pits.

In `src/ffb/FFBSafetyMonitor.h`:
```cpp
// Change this from 100.0f to 2.0f (Takes 0.5s to fade from 1.0 to 0.0)
static constexpr float SAFETY_SLEW_RESTRICTED = 2.0f;
```
