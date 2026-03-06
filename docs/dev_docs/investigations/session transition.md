
Here is a detailed report on how to robustly detect session changes, game phases, and player states for Le Mans Ultimate (LMU), utilizing the provided `SharedMemoryInterface.hpp`. 

This guide is written entirely from scratch, focusing purely on the logic and the specific data structures exposed by the LMU shared memory interface, ensuring no reliance on third-party plugin implementations.

---

# Robust Session and State Detection in Le Mans Ultimate

Tracking the exact state of a simulation can be tricky because data buffers update asynchronously, and session transitions can cause sudden data wipes. To build a robust application, you must combine **event-driven flags** with **state-polling logic** (edge detection).

Here is the recommended logic for implementing these features in your app.

### 1. Detecting Game Load and Main Menu States
To determine if the user is actively loaded into a track or sitting in the main menu, you should avoid guessing based on telemetry values (like RPM or speed). Instead, rely on the explicit event flags provided by the game.

*   **Data Source:** The `events` array inside the `SharedMemoryGeneric` struct.
*   **Logic:** 
    *   Monitor `events[SME_START_SESSION]` and `events[SME_END_SESSION]`. 
    *   When `SME_START_SESSION` is triggered, the track has loaded, and the simulation is active. You should initialize your app's internal session state.
    *   When `SME_END_SESSION` (or `SME_UNLOAD`) is triggered, the user has quit to the main menu or the server is changing tracks. You should pause telemetry processing and reset your UI.

### 2. Identifying the Session Type
Once a session has started, you need to identify whether it is a Practice, Qualifying, or Race session.

*   **Data Source:** `mSession` inside the `ScoringInfoV01` struct.
*   **Logic:** Read the integer value of `mSession` and map it to the corresponding session type:
    *   `0` = Testday
    *   `1` to `4` = Practice (1 through 4)
    *   `5` to `8` = Qualification (1 through 4)
    *   `9` = Warmup
    *   `10` to `13` = Race (1 through 4)

### 3. Handling Session Transitions Safely
A critical edge case occurs when a multiplayer server transitions directly from one session to another (e.g., Qualifying ends, and the Race begins). During this transition, the game engine will wipe the scoring buffers to prepare for the new session. If you are not careful, you will lose the final standings of the previous session.

*   **Robust Implementation Tip:** 
    *   Do not rely solely on polling the live `VehicleScoringInfoV01` array for final results.
    *   Instead, listen for the `events[SME_END_SESSION]` flag. 
    *   The exact moment this event evaluates to true, immediately perform a deep copy of the `VehicleScoringInfoV01` array into a safe "Previous Session Cache" in your app. This ensures you capture the final `mPlace` and `mFinishStatus` of all drivers before the `SME_START_SESSION` event fires and zeroes out the data for the upcoming race.

### 4. Tracking Race Phases (Garage to Green Flag)
While `mSession` tells you the *type* of event, `mGamePhase` tells you the *current chronological state* of that event.

*   **Data Source:** `mGamePhase` inside the `ScoringInfoV01` struct.
*   **Logic:** Map the integer to the following states:
    *   `0` = Before session has begun (Garage)
    *   `1` = Reconnaissance laps
    *   `2` = Grid walk-through
    *   `3` = Formation lap
    *   `4` = Starting-light countdown
    *   `5` = Green flag (Active Racing)
    *   `6` = Full course yellow / Safety car
    *   `7` = Session stopped (Red flag)
    *   `8` = Session over
*   **Robust Implementation Tip (Edge Detection):** To trigger events in your app (like a "Race Started" audio cue), you must implement edge detection. Store the `previousGamePhase` and compare it to the `currentGamePhase` on every tick. 
    *   *Example:* If `previousGamePhase == 4` and `currentGamePhase == 5`, you know the exact millisecond the green flag dropped.
    *   *Example:* If the phase transitions from `8` (Session over) to `0` (Before session), a session restart has occurred.

### 5. Player State: Monitor/UI vs. Driving
You need to differentiate between a player who is loaded into the server but looking at the setup monitor, versus a player who is physically sitting in the cockpit.

*   **Data Source:** The `events` array inside the `SharedMemoryGeneric` struct.
*   **Logic:** 
    *   Monitor `events[SME_ENTER_REALTIME]` and `events[SME_EXIT_REALTIME]`.
    *   When `SME_ENTER_REALTIME` fires, the player has clicked "Drive" and is in the car.
    *   When `SME_EXIT_REALTIME` fires, the player has pressed Escape and returned to the garage UI. 
    *   *Note:* Using these explicit events is vastly superior to checking if the car's speed is zero, as a car can be stationary on the track while the player is still in "Realtime".

### 6. Vehicle Location: Garage, Pit Lane, or On Track
Once the player is in the car (`SME_ENTER_REALTIME`), you need to know their physical location on the circuit.

*   **Data Source:** The `VehicleScoringInfoV01` struct (found inside the `SharedMemoryScoringData` array).
*   **Logic:** First, iterate through the vehicle array and find the vehicle where `mIsPlayer == true` (or use `playerVehicleIdx` from the telemetry struct). Once you have the player's vehicle object, evaluate the following booleans:
    *   **In the Garage:** If `mInGarageStall` is `true`, the car is physically spawned inside its designated garage box.
    *   **In the Pit Lane:** If `mInPits` is `true` AND `mInGarageStall` is `false`, the car is actively driving between the pit entrance and pit exit lines.
    *   **On the Track:** If `mInPits` is `false`, the car is out on the active racing surface.
*   **Granular Pit Tracking:** To know exactly what the pit crew is doing, read the `mPitState` variable:
    *   `0` = None (Not pitting)
    *   `1` = Request (Player has requested a stop via the HUD)
    *   `2` = Entering (Driving down the lane)
    *   `3` = Stopped (In the box, currently being serviced)
    *   `4` = Exiting (Service complete, leaving the box)

---

### Summary Checklist for your Application Loop

To build a bulletproof update loop in your app, follow this hierarchy on every tick:

1.  **Check Global State:** Read `events[SME_START_SESSION]` and `events[SME_END_SESSION]`. If the session is not active, skip all physics/scoring calculations.
2.  **Check Session Transitions:** If `mSession` changes, or `SME_END_SESSION` fires, cache the current `VehicleScoringInfoV01` array to preserve final results.
3.  **Check Realtime State:** Read `events[SME_ENTER_REALTIME]`. If the player is not in realtime, they are in the menus.
4.  **Update Game Phase:** Read `mGamePhase`. Compare it to the previous tick's phase to trigger start/stop/yellow flag logic.
5.  **Update Physical Location:** Find the player's vehicle (`mIsPlayer`). Read `mInGarageStall`, `mInPits`, and `mPitState` to update your app's track-position logic.

# Additional notes

The report above also includes tips on things like preserving race results, which are not relevant for our FFB app.
We only want to detect session transitions and start / finish to decide when to disable FFB and when to start and close a telemetry log with physics data.