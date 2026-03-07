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
