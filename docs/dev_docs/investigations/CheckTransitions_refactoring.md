# `GameConnector::CheckTransitions` — Refactoring

**Analysis date:** 2026-03-07  
**Implementation date:** 2026-03-07  
**Files changed:**
- `src/GameConnector.cpp`
- `src/GameConnector.h`
- `tests/test_gc_refactoring.cpp` *(new)*
- `tests/CMakeLists.txt`

**Related doc:** `docs/dev_docs/investigations/session transition.md`

**Status: ✅ Implemented and all tests passing (1812/1812 assertions)**

---

## 1. What the original code was doing

`CheckTransitions` was called every tick (from `CopyTelemetry`) and did two distinct things simultaneously inside a single 183-line monolithic function:

1. **State tracking** — reading new values from the shared memory snapshot and updating the atomic state machine members (`m_inRealtime`, `m_sessionActive`, `m_currentGamePhase`, `m_currentSessionType`, `m_playerControl`).
2. **Transition logging** — detecting when a tracked value *changed* against the `m_prevState` shadow copy, and logging a `[Transition]` line to the file log.

Both concerns were interleaved inside every `if`-block. For example:

```cpp
// 3. Game Phase (old code)
if (scoring.mGamePhase != m_prevState.gamePhase) {
    m_currentGamePhase = scoring.mGamePhase;  // ← state update, only on change
    // ... inline switch to build phaseStr ...
    Logger::Get().LogFile(...);               // ← logging
    m_prevState.gamePhase = scoring.mGamePhase;
}
```

Additional issues:
- `m_currentGamePhase` and `m_currentSessionType` were only updated when the value *changed* (inside the `!= prevState` guard), which was subtly wrong — the atoms lagged by one tick after reconnect.
- Five identical `switch` blocks for string lookup were duplicated inline.
- Two pairs of duplicate `Logger::Get().Log(...)` calls in `TryConnect` and `CheckLegacyConflict`.
- The "poll wins" design intent was implicit and easy to misread.

---

## 2. Identified problems (kept for reference)

### 2.1 Mixing state-update and logging concerns
The interleaved structure made it hard to answer: "When exactly is `m_currentGamePhase` updated?" and "Can I extend the logging without touching state?"

### 2.2 Two redundant tracking structures

| Concept | Atomic (public API) | `TransitionState` shadow (logging only) |
|---|---|---|
| In realtime | `m_inRealtime` | `m_prevState.inRealtime` |
| Game phase | `m_currentGamePhase` | `m_prevState.gamePhase` |
| Session type | `m_currentSessionType` | `m_prevState.session` |
| Player control | `m_playerControl` | `m_prevState.control` |
| Session active | `m_sessionActive` | (tracked from `trackName`) |

The atomics exist for thread-safe reads; the shadow exists only to detect changes for logging. Same values, different types and naming.

### 2.3 The "poll wins" hybrid was not explicit
The intent — "poll wins for final truth, events are fast-path updates" — was correct but not clearly expressed in the structure.

### 2.4 State atomics only updated inside change-detection guards
`m_currentGamePhase` and `m_currentSessionType` were only updated when `!= m_prevState`, so they could be stale by one tick immediately after connection.

### 2.5 Duplicate log calls
`Logger::Get().Log(...)` was duplicated on two consecutive lines in `TryConnect` and `CheckLegacyConflict`.

---

## 3. What was implemented

### ✅ 3.1 `CheckTransitions` split into two focused methods

`CheckTransitions` is now a thin orchestrator:

```cpp
void GameConnector::CheckTransitions(const SharedMemoryObjectOut& current) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    _UpdateStateFromSnapshot(current);  // Phase 1: sync state machine atomics
    _LogTransitions(current);           // Phase 2: log any changes
}
```

**`_UpdateStateFromSnapshot`** — runs unconditionally every tick. It first applies the ground-truth poll (all atomics updated from the buffer regardless of change), then applies any SME event overrides on top:

```cpp
void GameConnector::_UpdateStateFromSnapshot(const SharedMemoryObjectOut& current) {
    // --- Ground truth (polling) ---
    m_sessionActive      = (scoring.mTrackName[0] != '\0');
    m_inRealtime         = (scoring.mInRealtime != 0);
    m_currentGamePhase   = scoring.mGamePhase;   // always updated — no longer gated
    m_currentSessionType = scoring.mSession;      // always updated — no longer gated
    // ... playerControl from vehicle if present ...

    // --- Fast-path: SME event overrides on top of polled truth ---
    // Handles rapid transitions before the next poll tick arrives.
    for (int i = 0; i < SME_MAX; ++i) {
        if (generic.events[i] != m_prevState.eventState[i] && generic.events[i] != 0) {
            if (i == SME_START_SESSION)                     m_sessionActive = true;
            if (i == SME_END_SESSION || i == SME_UNLOAD) { m_sessionActive = false; m_inRealtime = false; }
            if (i == SME_ENTER_REALTIME)                    m_inRealtime = true;
            if (i == SME_EXIT_REALTIME)                     m_inRealtime = false;
        }
    }
}
```

**`_LogTransitions`** — has no side effects on state machine atomics. It only reads the snapshot and `m_prevState`, logs changes, and updates `m_prevState`.

### ✅ 3.2 Five static string-lookup helpers extracted

Declared as private `static` methods in `GameConnector.h`, implemented once in `GameConnector.cpp`:

| Helper | Replaces |
|---|---|
| `SmeEventName(int)` | inline `switch` in SME event loop |
| `GamePhaseName(unsigned char)` | inline `switch` in game phase block |
| `SessionTypeName(long)` | inline `if-else` chain in session block |
| `ControlModeName(signed char)` | inline `switch` in control block |
| `PitStateName(unsigned char)` | inline `switch` in pit state block |

### ✅ 3.3 `IsPlayerActivelyDriving()` composite predicate added

Implemented as a public inline method in `GameConnector.h`:

```cpp
bool IsPlayerActivelyDriving() const {
    return m_inRealtime.load(std::memory_order_relaxed)
        && m_playerControl.load(std::memory_order_relaxed) == 0  // Player (not AI/replay)
        && m_currentGamePhase.load(std::memory_order_relaxed) != 9; // 9 == Paused
}
```

This addresses the ESC-menu-while-on-track edge case from `session transition.md`: when a player presses ESC during a singleplayer session, `mGamePhase` becomes 9 (Paused). This predicate returns `false`, making it safe to suppress FFB in that state.

### ✅ 3.4 Duplicate log calls removed

- `TryConnect`: removed duplicate `Logger::Get().Log("[GameConnector] Could not map view of file.")` and `Logger::Get().Log("[GameConnector] Failed to init LMU Shared Memory Lock")`.
- `CheckLegacyConflict`: removed duplicate `Logger::Get().Log("[Warning] Legacy rFactor 2...")`.

### ⏭ 3.5 `GameState` struct consolidation — deliberately deferred

Grouping the five atomics into a single `GameState` struct was assessed as the highest-risk change (requires public API change for all callers of `IsSessionActive()`, `IsInRealtime()`, etc.) with no immediate correctness benefit. The other changes deliver the main readability wins. This can be done as a separate, focused refactoring step later.

---

## 4. Prioritized action plan — status

| Priority | Change | Status |
|---|---|---|
| 🟢 Low-hanging | Remove duplicate `Logger::Get().Log(...)` lines | ✅ Done |
| 🟢 Low-hanging | Extract `SmeEventName()` and other string helpers | ✅ Done |
| 🟡 Medium | Split `CheckTransitions` into `_UpdateStateFromSnapshot` + `_LogTransitions` | ✅ Done |
| 🟡 Medium | Add `IsPlayerActivelyDriving()` composite predicate | ✅ Done |
| 🔴 Larger | Consolidate atomics into `GameState` struct | ⏭ Deferred |

---

## 5. Tests written and passing

All tests are in `tests/test_gc_refactoring.cpp`, registered with the `refactoring` tag. All **13 tests / 1812 total assertions pass**.

### 5.1 Polling always wins

| Test | What it checks |
|---|---|
| `test_gc_poll_overrides_stale_realtime` | After `InjectTransitions` with `mInRealtime=0` and no exit event, `IsInRealtime()` → `false` (poll won over stale atomic state) |
| `test_gc_poll_overrides_stale_session` | After `InjectTransitions` with empty track name and no end-session event, `IsSessionActive()` → `false` |

### 5.2 Event fast-path still applies

| Test | What it checks |
|---|---|
| `test_gc_end_session_event_overrides_poll` | SME_END_SESSION fires while buffer still has track name + `mInRealtime=1`; both flags cleared by event override |

### 5.3 GamePhase / SessionType always updated

| Test | What it checks |
|---|---|
| `test_gc_gamephase_updated_unconditionally` | `GetGamePhase()` reflects every snapshot value across 3 transitions, no missed updates |
| `test_gc_sessiontype_updated_unconditionally` | `GetSessionType()` likewise, no guard needed |

### 5.4 PlayerControl tracking

| Test | What it checks |
|---|---|
| `test_gc_player_control_from_vehicle` | Three-step transition (Player → AI → Nobody) via `InjectTransitions` |
| `test_gc_player_control_unchanged_when_no_vehicle` | `playerHasVehicle=false` → `GetPlayerControl()` not modified |

### 5.5 `IsPlayerActivelyDriving` (new)

| Test | What it checks |
|---|---|
| `test_gc_actively_driving_true` | `inRealtime=1`, `mControl=0`, `mGamePhase=5` → `true` |
| `test_gc_actively_driving_false_when_paused` | `mGamePhase=9` (ESC menu) → `false` despite inRealtime + Player control |
| `test_gc_actively_driving_false_when_ai` | `mControl=1` (AI) → `false` |
| `test_gc_actively_driving_false_when_not_realtime` | `mInRealtime=0` (garage UI) → `false` |

### 5.6 No duplicate logging

| Test | What it checks |
|---|---|
| `test_gc_no_duplicate_log_on_same_state` | Two consecutive `InjectTransitions` with identical data produce exactly 1 `[Transition] GamePhase:` log line |

### 5.7 SME event name lookup

| Test | What it checks |
|---|---|
| `test_gc_sme_event_name_lookup` | Each of the 12 known SME event constants produces the correct string label in the log |

---

## 6. What was NOT changed (by design)

- The **polling + events hybrid** approach is preserved exactly. "Ground truth poll" + "fast-path events" is the right answer for LMU's async shared memory. The two halves now live in clearly named sections of `_UpdateStateFromSnapshot`.
- The **`TransitionState` shadow struct** (`m_prevState`) is kept as-is. It correctly captures "previous values for logging purposes" and the separation between it and the state machine atomics is now much more visible.
- The **mutex usage** is unchanged. `CheckTransitions` acquires `m_mutex` at its entry. `_UpdateStateFromSnapshot` and `_LogTransitions` are private helpers called under the already-held lock.
- The **`GameState` struct consolidation** is deferred — it is a valid future improvement but is the highest-risk change and provides no correctness benefit over the current structure.

---

## 7. Known Limitation — "Quit to Main Menu" Detection

**Status: ⚠️ Bug confirmed, root cause partially understood, fix deferred pending more data.**

### 7.1 Problem statement

When the user quits **directly to the main menu** (without returning to the garage first), the session state indicator remains `true` ("Track loaded") even though no session is actually active. The app shows that a track is loaded when it should show the main menu / disconnected state.

This was discovered in the course of the refactoring work. The refactoring did not introduce the bug — it pre-existed — but the investigation made it visible.

**Does not occur when** returning to garage and then quitting. In that case the session state is correct.

### 7.2 What we know about the signals

#### `mTrackName` (current ground-truth for `m_sessionActive`)
When quitting to the main menu, LMU **does not zero** `mTrackName` in the shared memory buffer. Our local copy retains the last valid track name indefinitely. This makes `mTrackName` alone **insufficient** for detecting a main-menu return.

Note: `mTrackName` is only re-copied into our local snapshot when `SME_UPDATE_SCORING` is non-zero (see `CopySharedMemoryObj`). If LMU stops sending scoring updates, our copy is already stale. The open question is whether LMU *does* stop sending scoring updates at the main menu.

#### `SME_END_SESSION` / `SME_UNLOAD` (intended session-end signals)
These events are **not fired** when the user quits directly to the main menu. They work correctly for the normal return-to-garage → session-end path (multiplayer track change, end of session, etc.), but fail for direct quit-to-menu.

This is a confirmed LMU shared memory API gap.

#### `mOptionsLocation` (investigated, abandoned)
Candidate values:
- `0` = "Main UI"
- `1` = Track Loading
- `2` = Monitor / Garage
- `3` = On Track

**Outcome**: Although `mOptionsLocation = 0` is associated with "Main UI", it is also **0 during normal gameplay**. In testing, the field appeared to only be set to non-zero values transiently (during loading screen transitions) and returns to 0 during all stable game states including active driving. Using it as a continuous session indicator caused a catastrophic regression: `m_sessionActive` became false every tick while driving, causing "[Game] User exited driving session." to spam the log hundreds of times per session.

`mOptionsLocation` cannot be used as a reliable continuous indicator of game state. It may be valid only at the precise moment an `SME_ENTER` / `SME_EXIT` event fires.

#### `SME_EXIT` / `SME_ENTER` (generic UI events)
These fire during the quit-to-menu transition, but they **also fire** during many other transitions (loading screens, garage entry, etc.). They cannot distinguish "going to main menu" from "returning to garage".

Observed at quit-to-menu:
```
[Transition] Event: SME_EXIT (N)
[Transition] Event: SME_ENTER (N+7)
[Transition] Event: SME_EXIT (N+9)
[Transition] Event: SME_ENTER (N+5)
```
These are just incrementing counters on indices 0 (ENTER) and 1 (EXIT). No qualitative meaning.

#### `SME_EXIT_REALTIME` (potentially important)
**Key observation**: When returning to the *garage*, `SME_EXIT_REALTIME` fires. When quitting to the *main menu*, **`SME_EXIT_REALTIME` does NOT fire** — `mInRealtime` goes to `0` via polling only.

This is a potential distinguisher, but requires more data to confirm it holds in all cases. See open questions below.

### 7.3 Open questions

| # | Question | Why it matters |
|---|---|---|
| Q1 | Does `SME_UPDATE_SCORING` stop incrementing after quitting to the main menu? | If yes, scoring data (including `mTrackName`) becomes detectably stale via a timeout |
| Q2 | Does `mNumVehicles` drop to 0 after quitting to main menu? | Could be used as a reliable session-end indicator if it does |
| Q3 | Does `playerHasVehicle` → `false` after quitting to main menu? | Same — telemetry is no longer updated when there's no player car |
| Q4 | Does `SME_EXIT_REALTIME` reliably fire for garage return but NOT for quit-to-menu? | Would be a clean distinguisher with zero false positives |
| Q5 | Does `mOptionsLocation` change when the loading screen appears? What is its value at that moment? | Would help understand if it can be used transiently |

### 7.4 Additional debug logging to add

Before the next test session, add the following entries to `_LogTransitions` in `GameConnector.cpp`. These are permanent additions (not temporary), as they provide useful diagnostic information for all session transitions:

**a) Log `playerHasVehicle` changes:**
```cpp
// After the existing control/pit block, outside the vehicle presence guard:
bool currentHasVehicle = current.telemetry.playerHasVehicle;
if (currentHasVehicle != m_prevState.playerHasVehicle) {
    Logger::Get().LogFile("[Transition] PlayerHasVehicle: %s -> %s",
        m_prevState.playerHasVehicle ? "true" : "false",
        currentHasVehicle ? "true" : "false");
    m_prevState.playerHasVehicle = currentHasVehicle;
}
```

**b) Log `mNumVehicles` changes:**
```cpp
int currentNumVehicles = (int)scoring.mNumVehicles;
if (currentNumVehicles != m_prevState.numVehicles) {
    Logger::Get().LogFile("[Transition] NumVehicles: %d -> %d",
        m_prevState.numVehicles, currentNumVehicles);
    m_prevState.numVehicles = currentNumVehicles;
}
```

**c) Log a comprehensive snapshot when `mInRealtime` goes from `true` to `false`.**
This fires at every de-realtime event (return to garage OR quit to menu), and will capture all the signals we need to compare the two paths:
```cpp
// Inside the mInRealtime transition block, after the existing log line:
if (!currentInRealtime && m_prevState.inRealtime) {
    Logger::Get().LogFile(
        "[Transition] De-realtime snapshot: trackName='%s' optionsLoc=%d "
        "numVehicles=%d playerHasVehicle=%s smUpdateScoring=%u",
        scoring.mTrackName,
        (int)current.generic.appInfo.mOptionsLocation,
        (int)scoring.mNumVehicles,
        current.telemetry.playerHasVehicle ? "true" : "false",
        (unsigned)current.generic.events[SME_UPDATE_SCORING]);
}
```

> [!NOTE]
> Items (a) and (b) need corresponding fields added to `TransitionState m_prevState` in `GameConnector.h`: `bool playerHasVehicle = false;` and `int numVehicles = -1;`.

### 7.5 Manual test protocol

Add the debug logging above, build, and run two specific test sessions:

---

**Test A — Baseline (return to garage)**

1. Launch lmuFFB, launch LMU
2. Load any track → enter session → click **Drive** and go on track for ~10 seconds
3. Press **Escape** → select **Return to Monitor** (back to garage UI)
4. Wait 5 seconds in the garage monitor
5. Click **Drive** again → drive for ~5 seconds → press Escape → **Quit to Main Menu**
6. Wait 5 seconds at the main menu
7. Quit the app

**What to capture**: The complete `lmuffb_debug.log`. Pay particular attention to the two `[Transition] De-realtime snapshot:` lines — one for step 3 (return to garage) and one for step 5 (quit to menu).

---

**Test B — Minimum repro for the bug**

1. Launch lmuFFB, launch LMU
2. Load any track → enter session → click **Drive** and go on track for ~10 seconds
3. Press **Escape** → select **Quit to Main Menu** directly (skipping Return to Garage)
4. Wait 10 seconds at the main menu *without loading another track*
5. Quit the app
6. Also note what the lmuFFB **GUI shows** as the session state during step 4

**What to capture**: The complete `lmuffb_debug.log` AND a screenshot/description of the GUI state at step 4 (does it say "Track loaded"? What does the session status show?).

---

### 7.6 Expected analysis

After receiving the two logs, compare the `[Transition] De-realtime snapshot:` lines for **return-to-garage** (Test A, step 3) vs **quit-to-menu** (Test B / Test A step 5):

| Field | Hypothesis: garage | Hypothesis: main menu |
|---|---|---|
| `trackName` | non-empty | non-empty (stale) |
| `optionsLoc` | 0 (transient, probably) | 0 (same, unreliable) |
| `numVehicles` | > 0 (cars still in session) | **0** (no session) |
| `playerHasVehicle` | true (car still spawned) | **false** (car removed) |
| `smUpdateScoring` | > 0 (scoring still arriving) | **0 or stops** (no more updates) |

If `numVehicles = 0` **only** at the main menu (not at garage return), that becomes the reliable fix:
```cpp
// In _UpdateStateFromSnapshot:
m_sessionActive = (scoring.mTrackName[0] != '\0') && (scoring.mNumVehicles > 0);
```

If `playerHasVehicle = false` only at main menu, that's an alternative signal.

If `SME_EXIT_REALTIME` appears in the garage-return log but NOT in the quit-to-menu log, that confirms it as a differentiator and a sticky-flag approach becomes viable.

### 7.7 Test results (2026-03-07)

Both tests were run with the diagnostic logging described in 7.4. The two `[Diag] De-realtime snapshot:` lines were:

**Test A — Return to garage (03:37:17):**
```
[Diag] De-realtime snapshot: track='Silverstone Grand Prix Circuit - ELMS' optionsLoc=0 numVehicles=1 playerHasVehicle=true smUpdateScoring=16
```

**Test A — Quit to main menu (03:38:04) / Test B (03:40:26):**
```
[Diag] De-realtime snapshot: track='Silverstone Grand Prix Circuit - ELMS' optionsLoc=0 numVehicles=1 playerHasVehicle=true smUpdateScoring=16
```

**Every single hypothesis was wrong.** The two snapshots are byte-for-byte identical. LMU leaves all scoring data completely stale when quitting to the main menu:

| Field | Hypothesis | Actual result |
|---|---|---|
| `numVehicles` | 0 at main menu | **1 (stale) in both cases** |
| `playerHasVehicle` | false at main menu | **true (stale) in both cases** |
| `smUpdateScoring` | 0 or stops | **16 in both cases** |
| `SME_EXIT_REALTIME` | fires for garage only | **does NOT fire in either case** |

The only observable difference is in the events that follow **after** the de-realtime snapshot:

- **Garage return:** only `SME_STARTUP` events appear (every 5 seconds), no `SME_ENTER`.
- **Quit to main menu:** `SME_ENTER` fires within **0–1 seconds** of the de-realtime transition, confirmed in both logs.

GUI state at main menu (Test B, confirmed by user): `Sim: Track Loaded | Session: Practice / State: In Menu | Control: AI`.

### 7.8 Fix implemented

A `m_pendingMenuCheck` flag was added to the state machine. The logic:

1. **Arm**: when polling detects `mInRealtime: true → false` while a session is active.
2. **Trigger** (→ `m_sessionActive = false`): if `SME_ENTER` fires while the flag is armed (within a 3-second deadline).
3. **Cancel**: if `mInRealtime` goes back to `true` before the deadline (user clicked Drive — still in a session).
4. **Expire**: if neither happens within 3 seconds — user is in the garage monitor; session stays active.

Two new tests were added in `tests/test_gc_refactoring.cpp`:
- `test_gc_main_menu_ends_session_via_sme_enter` — verifies the fix.
- `test_gc_garage_return_keeps_session_active` — verifies no regression on the working path.

**Status: ✅ Implemented. All 440 tests pass (1818/1818 assertions).**

---

## 8. Design consideration — Is session state actually needed?

### 8.1 The core question

The investigation revealed that accurately tracking `IsSessionActive()` is surprisingly hard: LMU's API has gaps (missing events, stale buffers), and fixing them requires heuristics with timing windows. This raises the question: **is session state actually necessary for the app's core function?**

The app has two primary jobs:
1. **Enable/disable FFB** — apply force-feedback when the player is actively driving, suppress it otherwise.
2. **Start/close the telemetry log file** — begin recording physics data when driving starts, close it gracefully when driving ends.

Both of these are already driven by `IsPlayerActivelyDriving()`, which depends on:
- `m_inRealtime` — is the player physically in the car?
- `m_playerControl == 0` — is the player (not AI/replay) in control?
- `m_currentGamePhase != 9` — is the game not in the ESC-menu paused state?

None of these check `m_sessionActive`.

### 8.2 What session state is currently used for

`IsSessionActive()` is primarily used for **GUI display** — showing "Sim: Track Loaded" vs "Sim: Disconnected" or "Sim: Main Menu". It is a UX convenience, not a functional requirement for FFB or logging.

### 8.3 Recommendation

Keep the session state detection and the quit-to-menu fix (it makes the GUI accurate and the state machine easier to reason about). But when evaluating the complexity of future edge cases, keep in mind:

> **If a heuristic fix for session state is "good enough" (occasionally stale for a few seconds), that is acceptable because `IsPlayerActivelyDriving()` is the actual gate for every FFB and logging decision.** Session state being briefly wrong never causes incorrect FFB output or corrupt log files — it only affects the status line in the GUI.

This means: do not over-invest in making `IsSessionActive()` pixel-perfect. The robustness budget is better spent on `IsPlayerActivelyDriving()` and `m_inRealtime` reliability, which directly affect user experience.

### 8.4 Code Audit Verification

To align the application with the considerations above, an audit and refactoring of `main.cpp` was carried out:
- A local, incomplete `is_driving` check that lacked the `gamePhase != 9` (paused) constraint was removed and completely delegated to `GameConnector::Get().IsPlayerActivelyDriving()`.
- A redundant short-circuit `(!is_session_active && was_driving)` in the logging exit logic was removed.

As of v0.7.142, both **FFB Output Authorization** and **Log File Lifecycles** are perfectly locked to the exact same predicate:
1. `should_start_log` happens exclusively when `IsPlayerActivelyDriving()` transitions `false → true`.
2. `should_stop_log` happens exclusively when `IsPlayerActivelyDriving()` transitions `true → false`.
3. `full_allowed` for physics calculation requires `IsPlayerActivelyDriving()` to be true.

`IsSessionActive()` is now completely decoupled from application logic and only serves as an indicator for the diagnostic Health Monitor / GUI display.
