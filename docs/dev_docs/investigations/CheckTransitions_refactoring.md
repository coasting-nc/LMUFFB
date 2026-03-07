# `GameConnector::CheckTransitions` — Refactoring Analysis

**Date:** 2026-03-07  
**File under review:** `src/GameConnector.cpp` — `GameConnector::CheckTransitions`  
**Related doc:** `docs/dev_docs/investigations/session transition.md`  

---

## 1. Summary of the current code

`CheckTransitions` is called every tick (from `CopyTelemetry`) and does two distinct things at once:

1. **State tracking** — reads new values from the shared memory snapshot and updates the atomic state machine members (`m_inRealtime`, `m_sessionActive`, `m_currentGamePhase`, `m_currentSessionType`, `m_playerControl`).
2. **Transition logging** — detects when a tracked value *changes* against the `m_prevState` shadow copy, and logs a `[Transition]` line to the file log.

Both concerns are interleaved inside every `if`-block, making the function harder to read and modify independently.

---

## 2. Identified problems

### 2.1 Mixing state-update and logging concerns

In every block the code does at least two things:

```cpp
// 3. Game Phase
if (scoring.mGamePhase != m_prevState.gamePhase) {
    m_currentGamePhase = scoring.mGamePhase;  // ← state update
    // ... build phaseStr ...
    Logger::Get().LogFile(...);               // ← logging
    m_prevState.gamePhase = scoring.mGamePhase; // ← prev-state update
}
```

This makes it hard to answer questions like:
- "When exactly is `m_currentGamePhase` updated?" 
- "Can I extend the logging without accidentally touching state?"

### 2.2 The atomic "state machine" and the `m_prevState` shadow are partially redundant

There are **two separate tracking structures** for the same logical concept:

| Concept | Atomic (public API) | `TransitionState` shadow (logging only) |
|---|---|---|
| In realtime | `m_inRealtime` | `m_prevState.inRealtime` |
| Game phase | `m_currentGamePhase` | `m_prevState.gamePhase` |
| Session type | `m_currentSessionType` | `m_prevState.session` |
| Player control | `m_playerControl` | `m_prevState.control` |
| Session active | `m_sessionActive` | (no shadow, detected from `trackName`) |

The atomics exist for thread-safe read from other threads. The shadow exists only to detect changes for logging. They track the same values but with different types and naming conventions, which can lead to drift.

### 2.3 State machine polled vs event-driven: the current hybrid is not explicit

The "robust fallback" comment at the top of the function is correct in intent:

```cpp
// Robust Fallback: Synchronize state flags with current buffer data (#274)
m_sessionActive = (scoring.mTrackName[0] != '\0');
m_inRealtime    = (scoring.mInRealtime != 0);
```

But then in the SME event loop, those same fields are set again via event:

```cpp
if (i == SME_START_SESSION) m_sessionActive = true;
if (i == SME_ENTER_REALTIME) m_inRealtime = true;
```

And later in section 2 (InRealtime) the logging shadow is updated from the *local variable* `currentInRealtime`, not from the atomic `m_inRealtime`. This is correct but subtle and easy to get wrong.

The intent — "poll wins for final truth, events are fast-path updates" — is good but not made explicit in code structure.

### 2.4 Logging a transition that has already been overwritten

Consider this order of operations for `m_inRealtime`:
1. At the top: `m_inRealtime = (scoring.mInRealtime != 0)` — **atomic updated immediately**.
2. In SME loop: `if (i == SME_ENTER_REALTIME) m_inRealtime = true` — may contradict.
3. In section 2: `currentInRealtime` is re-derived from raw scoring data and compared to `m_prevState.inRealtime` for logging.

There is no problem with the current correctness, but the atomics are updated *before* the logging comparison block. If a future developer reads section 2 and thinks "`m_prevState.inRealtime` vs `m_inRealtime`" is the comparison, that would be wrong. The redundancy creates a subtle trap.

### 2.5 Minor: duplicate log calls in `TryConnect`

```cpp
Logger::Get().Log("[GameConnector] Could not map view of file.");
Logger::Get().Log("[GameConnector] Could not map view of file.");  // duplicate
```

This appears twice for both the `MapViewOfFile` error and the `MakeSafeSharedMemoryLock` error. These are likely copy-paste leftovers and should be removed.

---

## 3. Proposed refactoring

### 3.1 Separate "poll state update" from "log transitions"

Split `CheckTransitions` into two private methods:

```
_UpdateStateFromSnapshot(current)   — updates atomics unconditionally every tick
_LogTransitions(current)            — compares m_prevState to current, logs changes
```

`CheckTransitions` becomes a thin orchestrator:

```cpp
void GameConnector::CheckTransitions(const SharedMemoryObjectOut& current) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    _UpdateStateFromSnapshot(current);  // fast, always runs
    _LogTransitions(current);           // slower; compares and updates m_prevState
}
```

Benefits:
- Each method has a single job.
- `_UpdateStateFromSnapshot` can later be tested independently.
- The logging path can be disabled or redirected without risk to state correctness.

### 3.2 Consolidate the state machine into a single struct — `GameState`

Instead of four separate atomics, introduce a `GameState` struct. It can still be updated atomically via a mutex, or individual fields can remain atomics — the key is that all state variables are grouped together with consistent naming.

```cpp
struct GameState {
    bool sessionActive = false;
    bool inRealtime    = false;
    long sessionType   = -1;
    unsigned char gamePhase  = 255;
    signed char   playerControl = -2;

    // Helper accessors for readable intent
    bool IsInMenu() const { return !inRealtime; }
    bool IsRacing() const { return inRealtime && sessionActive; }
};
```

In the `GameConnector` header:
```cpp
// Replace the five separate atomics:
// std::atomic<bool> m_sessionActive { false };
// std::atomic<bool> m_inRealtime    { false };
// etc.

// With a single locked struct:
GameState m_state;   // protected by m_mutex (already held wherever it's written)

// Public API becomes:
bool IsSessionActive() const { std::lock_guard<...> lk(m_mutex); return m_state.sessionActive; }
bool IsInRealtime()    const { std::lock_guard<...> lk(m_mutex); return m_state.inRealtime; }
```

**Trade-off:** The current atomics allow lock-free reads from other threads. If that matters for performance, keep atomics but group them inside a `GameStateAtomics` sub-struct inside `GameConnector`.

Even without removing atomics, renaming them to match the `TransitionState` shadow fields improves readability:

| Now (atomic) | Now (shadow) | Proposed unified name |
|---|---|---|
| `m_inRealtime` | `m_prevState.inRealtime` | `inRealtime` |
| `m_currentGamePhase` | `m_prevState.gamePhase` | `gamePhase` |
| `m_currentSessionType` | `m_prevState.session` | `sessionType` |
| `m_playerControl` | `m_prevState.control` | `playerControl` |

### 3.3 Make the "poll wins" pattern explicit

Add a single unconditional "ground truth" update block at the start of `_UpdateStateFromSnapshot`, clearly named and documented:

```cpp
// Ground truth (polling): overwrites any event-driven value with what the
// shared memory currently says. Events are a fast-path short-cut only.
void GameConnector::_UpdateStateFromSnapshot(const SharedMemoryObjectOut& snap) {
    auto& sc = snap.scoring.scoringInfo;
    m_state.sessionActive = (sc.mTrackName[0] != '\0');
    m_state.inRealtime    = (sc.mInRealtime != 0);
    m_state.gamePhase     = sc.mGamePhase;
    m_state.sessionType   = sc.mSession;
    if (snap.telemetry.playerHasVehicle) {
        uint8_t idx = snap.telemetry.playerVehicleIdx;
        if (idx < 104)
            m_state.playerControl = snap.scoring.vehScoringInfo[idx].mControl;
    }

    // Fast-path: apply event-driven overrides on top of the polled truth.
    // (These handle rapid transitions before the next poll tick arrives.)
    _ApplySmeEvents(snap.generic);
}
```

### 3.4 Consider a helper function for SME-name lookup

The `switch` inside the SME loop produces a string name. This can be extracted:

```cpp
static const char* SmeEventName(int i) {
    switch (i) {
        case SME_ENTER:             return "SME_ENTER";
        case SME_EXIT:              return "SME_EXIT";
        case SME_STARTUP:           return "SME_STARTUP";
        case SME_SHUTDOWN:          return "SME_SHUTDOWN";
        case SME_LOAD:              return "SME_LOAD";
        case SME_UNLOAD:            return "SME_UNLOAD";
        case SME_START_SESSION:     return "SME_START_SESSION";
        case SME_END_SESSION:       return "SME_END_SESSION";
        case SME_ENTER_REALTIME:    return "SME_ENTER_REALTIME";
        case SME_EXIT_REALTIME:     return "SME_EXIT_REALTIME";
        case SME_INIT_APPLICATION:  return "SME_INIT_APPLICATION";
        case SME_UNINIT_APPLICATION:return "SME_UNINIT_APPLICATION";
        default:                    return "Unknown";
    }
}
```

Similarly for `GamePhase`, `SessionType`, `ControlMode`, `PitState`.

### 3.5 Edge case from `session transition.md`: ESC-menu while on track

The investigation doc identifies a gap: when the player presses ESC while on track (showing setup/controls menu), the car is still physically on track but we ideally want to pause FFB. The current code tracks `mControl` (Player/AI) which *may* help, but whether AI takes over during ESC menus depends on session type (paused in offline, running in online).

**Suggestion:** Add a dedicated flag for "player visible in cockpit" that combines:
- `mInRealtime == true` (in the car, not in garage UI)
- `mControl == 0` (Player, not AI-controlled)
- `mGamePhase != 9` (not paused) — for offline, this will be 9 during ESC menus

This composite flag is a good candidate to be a method on the proposed `GameState` struct:

```cpp
bool IsPlayerActivelyDriving() const {
    return inRealtime
        && playerControl == 0   // Player-controlled (not replay/AI)
        && gamePhase != 9;      // Not paused
}
```

This addresses an existing gap without adding new shared memory reads.

---

## 4. Prioritized action plan

| Priority | Change | Effort | Risk |
|---|---|---|---|
| 🟢 Low-hanging | Remove duplicate `Logger::Get().Log(...)` lines in `TryConnect` | < 5 min | None |
| 🟢 Low-hanging | Extract `SmeEventName()` (and similar) as static helpers | ~15 min | None |
| 🟡 Medium | Split `CheckTransitions` into `_UpdateStateFromSnapshot` + `_LogTransitions` | ~1h | Low |
| 🟡 Medium | Add `IsPlayerActivelyDriving()` composite predicate | ~30 min | Low |
| 🔴 Larger | Consolidate atomics into `GameState` struct | ~2h | Medium (API change) |

---

## 5. What NOT to change

- The **polling + events hybrid** approach is correct as described in `session transition.md`. The "ground truth poll" + "fast-path events" pattern is the right answer for LMU's async shared memory. Do not remove either half of it.
- The **`TransitionState` shadow struct** is a good pattern and should be kept (or merged into the proposed `GameState` approach). It correctly captures "previous values for logging purposes".
- The **mutex usage** is correct. `CheckTransitions` is called with the lock already held from `CopyTelemetry`. This should be preserved.
