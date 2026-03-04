# LMU Shared Memory Transitions Analysis

This report investigates the variables within the LMU Shared Memory Interface that provide information regarding game state transitions (e.g., loading, changing menus, starting sessions, starting driving, pausing, etc.), and proposes strategies for robustly detecting and handling these transitions to prevent FFB jolts and bugs.

## 1. Relevant State Variables

The shared memory interface provides multiple structs containing simulation context. Here are all the variables that give info about when a transition happens:

### A. SharedMemory Events (`SharedMemoryGeneric::events`)
From `SharedMemoryInterface.hpp`, the interface exposes an array of event triggers, which represent high-level transitions fired by the internal engine:
- `SME_ENTER` / `SME_EXIT`
- `SME_STARTUP` / `SME_SHUTDOWN`
- `SME_LOAD` / `SME_UNLOAD`
- `SME_START_SESSION` / `SME_END_SESSION`
- `SME_ENTER_REALTIME` / `SME_EXIT_REALTIME` (Realtime is when the user jumps into the car)
- `SME_UPDATE_SCORING` / `SME_UPDATE_TELEMETRY`
- `SME_INIT_APPLICATION` / `SME_UNINIT_APPLICATION`
- `SME_SET_ENVIRONMENT`

### B. Application State (`ApplicationStateV01`)
This is the most reliable indicator of where the user interface currently resides.
- `mOptionsLocation`:
  - `0` = Main UI
  - `1` = Track Loading
  - `2` = Monitor (Pits Menu / Garage)
  - `3` = On Track (Driving or in the overlay pause menu)

### C. Scoring Info (`ScoringInfoV01`)
- `mInRealtime`: `true` if currently in the car realtime loop, `false` if in the monitor UI. Maps closely to `SME_ENTER_REALTIME`.
- `mSession`: `0`=testday, `1-4`=practice, `5-8`=qual, `9`=warmup, `10-13`=race. Detects transition between weekend phases.
- `mGamePhase`: Indicates session progression and pause state.
  - `0` = Before session has begun
  - `1` = Reconnaissance laps (race only)
  - `2` = Grid walk-through (race only)
  - `3` = Formation lap (race only)
  - `4` = Starting-light countdown has begun (race only)
  - `5` = Green flag
  - `6` = Full course yellow / safety car
  - `7` = Session stopped
  - `8` = Session over
  - `9` = **Paused** (Specifically acts as a heartbeat call to the plugin when paused)

### D. Vehicle Scoring Info (`VehicleScoringInfoV01`)
- `mControl`: Identifies exactly who is driving the car.
  - `-1`=nobody, `0`=local player, `1`=local AI, `2`=remote, `3`=replay
  - *Tip: Transitioning from `0` to `1` usually causes FFB spikes as AI forces the steering onto a racing line instantly.*
- `mPitState`: `0`=none, `1`=request, `2`=entering, `3`=stopped, `4`=exiting. Useful to determine pit box transitions.

### E. Telemetry Info (`TelemInfoV01`)
- `mElapsedTime`, `mDeltaTime`: Simulation temporal metrics. When `mDeltaTime == 0.0` or `mElapsedTime` stops incrementing repeatedly, the simulation is hard-paused.
- `mPhysicalSteeringWheelRange`: Sometimes this changes on pause menus if the player tweaks steering lock settings while driving paused.

---

## 2. Transition Mapping Examples

By interpreting combinations of the variables above:

- **Game Started / Entering Main Menu:** `mOptionsLocation` is `0`.
- **Track Loading:** `mOptionsLocation` is `1` (often paired with `events[SME_LOAD]`).
- **In Garage / Start Session:** `mOptionsLocation` is `2`, `mInRealtime` is `false`.
- **Started Driving (Clicked "Drive"):** `mOptionsLocation` switches from `2` to `3`, `mInRealtime` goes `true`, and `events[SME_ENTER_REALTIME]` fires.
- **Pausing While Driving (ESC):** `mOptionsLocation` usually remains `3`, but `mGamePhase` changes to `9`. Alternatively, the physics thread completely stops, which means `mDeltaTime` and `mCurrentET` stagnate.
- **Changing Settings While Paused:** Monitored while the game is paused (`mGamePhase == 9`). If the user clicks out to the Monitor, `mOptionsLocation` will jump to `2`.

## 3. Transition Logging Requirements

To facilitate robust debugging, the application should maintain a **transition trace** within the debug log file (output to file, but omitted from the standard console to prevent clutter). 

This debug trace helps identify the exact state of the game immediately prior to a bug or crash occurring. 

**Implementation Desiderata:**
1. **Event Tracing:** Every time a discrete transition is detected (e.g., entering driving, pausing, returning to pits, AI taking control), a timestamped entry should be written to the log file.
2. **State Snapshot:** The log entry must include a brief snapshot of the relevant variables (e.g., `[Transition] Unpaused: mOptionsLocation=3, mGamePhase=5, mInRealtime=true, mControl=0`).
3. **Throttling Considerations:** Frequent or continuous variables (like `mDeltaTime` tracking, or `SME_UPDATE_TELEMETRY`) MUST NOT be logged here. Only atomic state changes should generate a log entry to avoid spamming the file and causing performance hits from disk I/O.

---

## 4. Update Frequency Analysis of State Variables

To ensure the debug logs do not become spammed, it is critical to classify variables based on how frequently they change:

### A. Occasional / Discrete Changes (Safe to Log on Change)
These variables only change based on direct user input, deliberate game state progression, or menu navigation. They are ideal triggers for logging:
- **`SharedMemoryGeneric::events` (High-Level Only):** `SME_ENTER`, `SME_EXIT`, `SME_STARTUP`, `SME_SHUTDOWN`, `SME_LOAD`, `SME_UNLOAD`, `SME_START_SESSION`, `SME_END_SESSION`, `SME_ENTER_REALTIME`, `SME_EXIT_REALTIME`, `SME_INIT_APPLICATION`, `SME_UNINIT_APPLICATION`. (Fires exactly once per corresponding transition).
- **`ApplicationStateV01::mOptionsLocation`**: Changes only when navigating between major menus (Main UI, Loading, Garage, Driving).
- **`ScoringInfoV01::mInRealtime`**: Changes only when jumping in/out of the car.
- **`ScoringInfoV01::mSession`**: Changes only between phases of a race weekend (Practice -> Quali -> Race).
- **`ScoringInfoV01::mGamePhase`**: Progresses through distinct race phases. The `9` (Paused) state turns on/off explicitly upon pressing the pause button.
- **`VehicleScoringInfoV01::mControl`**: Changes only if the player gives/takes control to/from the AI or remote server.
- **`VehicleScoringInfoV01::mPitState`**: Iterates through 5 steps sequentially during a pit stop event.
- **`TelemInfoV01::mPhysicalSteeringWheelRange`**: Changes occasionally if a setup or specific option adjustment occurs.

### B. Frequent / Continuous Changes (DO NOT Log on Change)
These variables are updated continuously by the simulation logic or physics engine (e.g., hundreds of times per second). Logging these raw events directly will immediately spam the log file:
- **`SharedMemoryGeneric::events` (Continuous):**
  - **`SME_UPDATE_TELEMETRY`**: Extremely frequent. Fires every physics frame. **Do not log this event.**
  - **`SME_UPDATE_SCORING`**: Extremely frequent. Fires every scoring update (many times a second). **Do not log this event.**
  - **`SME_SET_ENVIRONMENT`**: Usually occasional, but can potentially fire rapidly if weather conditions interpolate.
- **`TelemInfoV01::mElapsedTime`**, **`TelemInfoV01::mDeltaTime`**: Changing literally every frame. These should be *polled* silently to detect "freezes", but the raw numbers should never be logged sequentially.

---

## 5. Suggestions for Robust Transition Detection

Relying entirely on a single boolean or event array isn't always reliable (e.g., dropped frames or decoupled physics/UI threads). You should use a **Compound State Machine** to determine transitions safely. 

Here are additional ways to robustly catch and handle these transitions to avoid FFB jolts:

### A. Polled Value Comparison (State Matrix)
Keep a struct caching the *previous* frame's values: `prevOptionsLocation`, `prevGamePhase`, `prevInRealtime`, `prevControl`. On each telemetry tick, detect edges:
```cpp
if (prevOptionsLocation == 2 && current.appInfo.mOptionsLocation == 3) {
    HandleEnteredDriving();
}
if (prevGamePhase != 9 && current.scoring.scoringInfo.mGamePhase == 9) {
    HandlePaused();
}
```
This is inherently safer than trusting `SME_` flags, because it catches the exact data footprint without missing atomic events.

### B. Detect Stale physics ("Hard Freeze/Pause")
Sometimes LMU blocks entirely (like loading a config or stuttering) and doesn't fire `mGamePhase = 9` right away. To prevent FFB algorithms from generating infinite torque build-ups due to bad `mDeltaTime` accumulations:
- Check if `current.scoring.scoringInfo.mCurrentET == prevCurrentET`.
- If Elapsed Time stalls for more than 2-3 cycles, assume a pause and **cut motor force / zero FFB**.

### C. Detect Missing Hardware Reads (Timeouts)
Sometimes the shared memory mapping doesn't receive updates if LMU stutters or crashes.
- Monitor `WaitMultipleObjects` timeouts in the `SharedMemoryInterface` loop wrapper. A timeout `WAIT_TIMEOUT` larger than normal physics refresh intervals implies a frozen transition or lag.

### D. FFB "Slewing" on Transitions (Zero-Jolt Logic)
Spikes usually happen when the physics engine instantly repositions the car (pits, AI swap, unpausing).
- When a transition is detected (e.g., unpausing / `mGamePhase` leaving `9`, jumping into the car / `mOptionsLocation` -> `3`, `mControl` -> `1`), apply a software **FFB gain multiplier** that starts at 0.0 and safely linear-ramps to 1.0 over ~500-1000ms.
- This effectively mutes any immediate 100% torque spikes triggered by the steering column catching up to a new wheel rotation offset.

### E. User Settings Drift Detection
If the user changes settings while paused (like physical steering lock or visual range), compare `prevSteeringLock` with `currentSteeringLock` every frame. 
If `mGamePhase == 9` and you detect a settings shift, wait for the unpause transition (`mGamePhase` leaving `9`) to apply the new configurations to the wheel base smoothly.
