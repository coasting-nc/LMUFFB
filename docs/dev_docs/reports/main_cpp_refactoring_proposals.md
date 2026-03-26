# Refactoring Proposals: `src/core/main.cpp`

> **Date:** 2026-03-26  
> **Version at time of analysis:** 0.7.x  
> **Status:** Proposal — no code changes included in this document

---

## 1. Executive Summary

`main.cpp` currently serves three distinct roles simultaneously:

1. **Application bootstrap** — argument parsing, logger init, GUI init, device init, shutdown sequencing.
2. **FFB physics & upsampling loop** — the 1000 Hz `FFBThread` function, including the upsampling phase accumulator, lost-frame detection, session-logging lifecycle, and health monitoring.
3. **Global state definitions** — the six globals (`g_running`, `g_ffb_active`, etc.) and their `#ifdef LMUFFB_UNIT_TEST` extern mirrors.

The file is only 392 lines, which is manageable, but the *responsibilities* are deeply tangled, making it hard to test the physics/FFB logic in isolation, hard to reason about the startup sequence, and hard to evolve either concern without touching the other.

The proposals below are ordered from lowest to highest invasiveness.

---

## 2. Code-Quality Issues (Quick Wins)

### 2.1 Duplicate Log Lines ✅ _Implemented in v0.7.255_

**Lines 329–330 and 334–335** previously called `Logger::Get().Log(...)` twice with the same string. Both duplicates have been removed. The GUI-failure message was additionally promoted from `Log()` to `LogFile()` so it is persisted to disk in case of a crash.

---

### 2.2 Magic Numbers ✅ _Implemented in v0.7.255_

All numeric literals listed below have been replaced with named `constexpr` constants in an anonymous namespace at the top of the file:

| Constant | Value | Meaning |
|---|---|---|
| `kFFBTargetHz` | `1000` | FFB loop target frequency (Hz) |
| `kPhysicsUpsampleM` | `2` | Upsampling `M` coefficient |
| `kPhysicsUpsampleL` | `5` | Upsampling `L` coefficient |
| `kPhysicsTimestepSec` | `0.0025` | Physics timestep (s) |
| `kStaleThresholdMs` | `100` | Staleness threshold (ms) |
| `kMaxVehicleIndex` | `104` | Max vehicle index (game-imposed limit) |
| `kHealthWarnCooldownS` | `60` | Health warning cooldown (seconds) |
| `kGuiPeriodMs` | `16` | GUI loop period (ms, ≈60 Hz) |

> [!NOTE]
> `kPhysicsTimestepSec` (1/400 Hz) and `kPhysicsUpsampleL/M` are directly related to `PolyphaseResampler`'s design and should ideally be declared in `UpSampler.h` as `constexpr` members so they're co-located with the algorithm they describe. This is tracked as §4.2.

---

### 2.3 `static` Locals Inside a Hot Loop ✅ _Implemented in v0.7.255_

All four `static` local variables in `FFBThread` have been converted to plain (non-static) locals:

```cpp
// Before (static locals — fragile in tests)
static bool was_driving = false;
static long last_session = -1;
static double last_telem_et = -1.0;
static auto lastWarningTime = TimeUtils::GetTime();

// After (ordinary locals — reset on every thread launch)
bool was_driving = false;
long last_session = -1L;
double last_telem_et = -1.0;
auto lastWarningTime = TimeUtils::GetTime();
```

Since `FFBThread` is not restarted mid-run in production, this is zero-risk functionally, and it makes state scoping explicit. Test suites no longer risk state bleed between test cases through these statics.

---

### 2.4 `g_running = false` After `ffb_thread.joinable()` Check ✅ _Implemented in v0.7.255_

A 3-line comment has been added to the shutdown path explaining that `g_running` is already `false` at this point (the GUI `while` loop exits precisely when it becomes `false`), but is set explicitly for clarity and defensive safety in case future code paths bypass the main loop.

---

### 2.5 Indentation Error in `lmuffb_app_main` ✅ _Implemented in v0.7.255_

The entire body of the `try {}` block in `lmuffb_app_main` has been re-indented by one level, aligning it consistently with the `#ifdef _WIN32 / timeBeginPeriod(1)` block above it. Purely cosmetic — no behaviour change.

---

## 3. Clean Code / Responsibility Separation

### 3.1 Extract Session-Log Lifecycle into a Helper

The driving/logging state machine (lines 121–165) is an 45-line block embedded in the middle of the physics path. It tracks `was_driving`, `last_session`, detects transitions, starts/stops `AsyncLogger`, and calls `PopulateSessionInfo`. This is cohesive session-lifecycle logic, not physics logic.

**Proposal:** Extract to a free function `UpdateSessionLogging(...)`:

```cpp
// In main.cpp (or a future SessionManager.h)
namespace {
void UpdateSessionLogging(
    bool is_driving,
    long current_session,
    bool& was_driving,   // in/out
    long& last_session,  // in/out
    const SharedMemoryObjectOut& localData,
    const FFBEngine& engine)
{
    // transition detection + AsyncLogger start/stop logic
}
} // anonymous namespace
```

**Benefits:**
- Physics loop becomes readable (a single `UpdateSessionLogging(...)` call replaces 45 lines).
- The function can be unit tested independently with mock inputs.
- State variables (`was_driving`, `last_session`) become explicit arguments rather than hidden `static` locals.

---

### 3.2 Extract Health Warning Emission

Lines 228–249 (health monitoring + log warning) are another distinct concern inside the physics tick. The `lastWarningTime` `static` local compounds the issue.

**Proposal:** Extract to:

```cpp
namespace {
void MaybeEmitHealthWarning(
    const HealthStatus& health,
    bool in_realtime,
    std::chrono::steady_clock::time_point& lastWarningTime)
{
    if (!in_realtime || health.is_healthy) return;
    auto now = TimeUtils::GetTime();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastWarningTime).count() < kHealthWarnCooldownS) return;
    // ... build and emit reason string
    lastWarningTime = now;
}
} // anonymous namespace
```

This converts the `static` into a named parameter, making the cooldown mechanism explicit and testable.

---

### 3.3 Extract Lost-Frame Detection

Lines 170–175 contain the lost-frame detection logic (Issue #303). This is self-contained and has its own "last seen ET" state.

```cpp
static double last_telem_et = -1.0;
if (g_engine.m_safety.m_config.stutter_safety_enabled && ...) {
    g_engine.m_safety.TriggerSafetyWindow("Lost Frames");
}
last_telem_et = ...;
```

**Proposal:** This belongs in `FFBSafetyMonitor`, not `main.cpp`. Move the stutter/frame-drop detection into the safety monitor itself, possibly as a `CheckForLostFrames(double currentET, double deltaTime)` method that internally tracks the previous ET. This hides implementation detail and keeps rate-sensitive logic near the safety subsystem it controls.

---

## 4. Separating Physics/FFB Concerns from `FFBThread` (Main Ask)

This is the most impactful architectural proposal. The current `FFBThread` function mixes:

| Concern | Lines |
|---|---|
| Tick-rate timing setup | 89–91 |
| Upsampling phase accumulator | 97–104 |
| Session & driving-state tracking | 112–165 |
| Lost-frame detection | 170–175 |
| Physics force calculation | 178–216 |
| Upsampler invocation | 255 |
| Rate pushback to engine | 258–266 |
| DirectInput hardware output | 268–270 |
| Sleep / timing precision | 272–282 |
| Health monitoring | 227–249 |

The proposal is to introduce a **`PhysicsLoop`** class (or struct) that encapsulates everything from line 79 to line 250, moving it out of `main.cpp`.

### 4.1 Proposed: `src/ffb/PhysicsLoop.h/.cpp`

```cpp
namespace LMUFFB {

/**
 * @brief Owns and runs the 400Hz physics sampling tick.
 *
 * Encapsulates: phase accumulation, upsample scheduling,
 * telemetry reads, force calculation, safety slew,
 * and diagnostic rate monitoring.
 */
class PhysicsLoop {
public:
    PhysicsLoop();

    /**
     * @brief Execute one 1000Hz step.
     *
     * Returns the upsampled force value to send to hardware.
     * Also updates rate metrics on FFBEngine for GUI visibility.
     *
     * @param engine   Shared engine state (guarded by caller's mutex).
     * @param localData Current shared memory snapshot.
     * @return Upsampled force in [-1.0, 1.0].
     */
    double Tick(FFBEngine& engine, const SharedMemoryObjectOut& localData);

    // Rate accessors for pushing to engine
    double GetLoopRate()    const;
    double GetPhysicsRate() const;
    // ...

private:
    PolyphaseResampler  m_resampler;
    int                 m_phase_accumulator = 0;
    double              m_current_physics_force = 0.0;

    RateMonitor m_loop_monitor;
    RateMonitor m_telem_monitor;
    RateMonitor m_hw_monitor;
    RateMonitor m_physics_monitor;
    RateMonitor m_torque_monitor;
    RateMonitor m_gen_torque_monitor;

    ChannelMonitors m_channel_monitors;

    // Persistent-across-ticks state (replacing static locals)
    double m_last_et          = -1.0;
    double m_last_torque      = -9999.0;
    float  m_last_gen_torque  = -9999.0f;
    double m_last_telem_et    = -1.0;
    std::chrono::steady_clock::time_point m_last_warning_time;

    // Session tracking (replacing static locals)
    bool m_was_driving    = false;
    long m_last_session   = -1;

    // Internal helpers
    void RunPhysicsTick(FFBEngine& engine, const SharedMemoryObjectOut& localData);
    void UpdateSessionLog(bool is_driving, long current_session,
                          const SharedMemoryObjectOut& localData,
                          const FFBEngine& engine);
    void CheckLostFrames(FFBEngine& engine, double current_et, double delta_time);
    void MaybeEmitHealthWarning(const HealthStatus& health, bool in_realtime);
};

} // namespace LMUFFB
```

The resulting `FFBThread` in `main.cpp` becomes approximately:

```cpp
void FFBThread() {
    Logger::Get().LogFile("[FFB] Loop Started.");
    PhysicsLoop physics;
    const std::chrono::microseconds target_period(kFFBTargetHz); // 1000µs
    auto next_tick = TimeUtils::GetTime();

    while (g_running) {
        next_tick += target_period;

        double force = 0.0;
        {
            std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
            force = physics.Tick(g_engine, g_localData);
        }

        if (DirectInputFFB::Get().UpdateForce(force)) {
            physics.RecordHwEvent();
        }

        std::this_thread::sleep_until(next_tick);
    }
    Logger::Get().LogFile("[FFB] Loop Stopped.");
}
```

**Benefits:**
- `FFBThread` drops from ~215 lines to ~20 lines.
- `PhysicsLoop::Tick` can be called directly in unit tests without spinning a thread.
- All "static locals" become clearly owned member variables.
- The resampler, phase accumulator, and rate monitors are co-located with the logic that drives them.

> [!IMPORTANT]
> `PhysicsLoop::Tick` must not acquire `g_engine_mutex` internally. The caller (`FFBThread`) holds the lock and passes a reference. This maintains the current locking discipline.

---

### 4.2 Where Should the Upsampling Constants Live?

Currently, the upsampling ratio (`L=5`, `M=2`) and the resulting 400 Hz physics tick rate are implicit: the ratio is encoded as the magic numbers `+= 2` and `>= 5` in `FFBThread`, while `PolyphaseResampler` itself documents "5/2" in its header comment.

**Proposal:** Expose them as `constexpr` members of `PolyphaseResampler` (or a companion `UpsamplerConfig` struct):

```cpp
// In UpSampler.h
class PolyphaseResampler {
public:
    static constexpr int kUpsampleL        = 5;   // Interpolation factor
    static constexpr int kUpsampleM        = 2;   // Decimation factor
    static constexpr int kOutputHz         = 1000;
    static constexpr int kInputHz          = kOutputHz * kUpsampleM / kUpsampleL; // 400
    static constexpr double kInputTimestep = 1.0 / kInputHz; // 0.0025s
    // ...
};
```

Then `main.cpp` (or `PhysicsLoop`) consumes them:

```cpp
phase_accumulator += PolyphaseResampler::kUpsampleM;
if (phase_accumulator >= PolyphaseResampler::kUpsampleL) { ... }

// And everywhere 0.0025 is used:
force_physics = g_engine.m_safety.ApplySafetySlew(force_physics, PolyphaseResampler::kInputTimestep, ...);
```

This makes the coupling between the upsampler design and the physics tick rate _explicit and type-safe_, rather than relying on comments.

---

## 5. Testability

### 5.1 The `#ifdef LMUFFB_UNIT_TEST` Pattern — Audit

The current test-mode split:

```cpp
#ifndef LMUFFB_UNIT_TEST
std::atomic<bool> g_running(true);
// ... definitions
#else
extern std::atomic<bool> g_running;
// ... externs (defined in test harness)
#endif
```

This pattern works, but it embeds test-mode infrastructure directly into production code. Every global added to `main.cpp` requires a matching line in both the `#ifndef` and `#else` branches.

**Proposal:** Collect all shared globals into a dedicated **`ApplicationState`** struct (see §5.2). This makes the mock surface smaller and explicitly documented.

---

### 5.2 Introduce `ApplicationState`

```cpp
// src/core/ApplicationState.h
namespace LMUFFB {
struct ApplicationState {
    std::atomic<bool>         running{true};
    std::atomic<bool>         ffb_active{true};
    SharedMemoryObjectOut     local_data{};
    FFBEngine                 engine{};
    std::recursive_mutex      engine_mutex{};
};
} // namespace LMUFFB
```

A single `ApplicationState g_app;` replaces the 5 separate globals. Tests instantiate their own `ApplicationState` and pass it by reference into `PhysicsLoop` and `FFBThread`, eliminating the need for the `extern` trick entirely.

> [!WARNING]
> `FFBEngine` has a non-trivial constructor and is currently not movable. Verify that wrapping it in a struct does not cause a performance regression on startup, and that the recursive_mutex inside a struct is portable on Linux.

---

### 5.3 `PopulateSessionInfo` Already Extracted — Good

`PopulateSessionInfo` (lines 31–48) is the one testability win already present. It is a free function, takes all inputs as parameters, has no side effects, and is covered by tests. This pattern should be applied broadly.

---

## 6. Architectural Context: Phase 6 Namespace Migration

The existing `main_code_unity_build_plan.md` (Phase 6) proposes moving modules into sub-namespaces (`LMUFFB::Logging`, `LMUFFB::Physics`, `LMUFFB::GUI`). The `PhysicsLoop` class proposed above fits naturally into `LMUFFB::Physics` (or `LMUFFB::FFB`), aligning the code with Phase 6 goals without blocking it.

Suggested sub-namespace assignment:

| Type | Sub-namespace |
|---|---|
| `PhysicsLoop` | `LMUFFB::FFB` |
| `PolyphaseResampler` | `LMUFFB::FFB` |
| `FFBEngine` | `LMUFFB::FFB` |
| `FFBSafetyMonitor` | `LMUFFB::FFB` |
| `HealthMonitor` | `LMUFFB::Logging` |
| `RateMonitor`, `ChannelMonitor` | `LMUFFB::Logging` |
| `ApplicationState` | `LMUFFB` (root) |

---

## 7. Prioritized Implementation Roadmap

| Priority | Proposal | Effort | Risk | Status |
|---|---|---|---|---|
| 🟢 **1** | Fix duplicate log lines (§2.1) | Trivial | Zero | ✅ Done (v0.7.255) |
| 🟢 **2** | Fix indentation in `lmuffb_app_main` (§2.5) | Trivial | Zero | ✅ Done (v0.7.255) |
| 🟢 **3** | Replace `static` locals with plain locals (§2.3) | Low | Low | ✅ Done (v0.7.255) |
| 🟢 **4** | Name the magic number constants (§2.2) | Low | Low | ✅ Done (v0.7.255) |
| 🟢 **4b** | Clarify `g_running` shutdown comment (§2.4) | Trivial | Zero | ✅ Done (v0.7.255) |
| 🟡 **5** | Expose upsample constants from `UpSampler.h` (§4.2) | Low–Medium | Low | Open |
| 🟡 **6** | Extract `UpdateSessionLogging` helper (§3.1) | Medium | Low | Open |
| 🟡 **7** | Extract `MaybeEmitHealthWarning` helper (§3.2) | Medium | Low | Open |
| 🟡 **8** | Move lost-frame detection into `FFBSafetyMonitor` (§3.3) | Medium | Medium | Open |
| 🔴 **9** | Introduce `PhysicsLoop` class (§4.1) | High | Medium | Open |
| 🔴 **10** | Introduce `ApplicationState` struct (§5.2) | High | Medium | Open |

---

## 8. Files Expected to Be Modified

| File | Action |
|---|---|
| `src/core/main.cpp` | Primary target — all proposals above |
| `src/ffb/UpSampler.h` | Add `constexpr` sampling constants |
| `src/ffb/FFBSafetyMonitor.h/.cpp` | Accept lost-frame detection migration |
| `src/ffb/PhysicsLoop.h/.cpp` | New file (proposal §4.1) |
| `src/core/ApplicationState.h` | New file (proposal §5.2), optional |

---

## 9. Timing Triage: Now vs. After Phase 6

> **Context (as of 2026-03-26):**
> Phases 1–5 of `main_code_unity_build_plan.md` are **complete**. All core source files are encapsulated inside `namespace LMUFFB` and whitelisted for Unity builds.
> Phase 6 (sub-namespace migration) is **in-progress**: `src/logging/` has been transitioned to `LMUFFB::Logging` (v0.7.253). Next targets are `src/utils/` → `LMUFFB::Utils` and `src/physics/` → `LMUFFB::Physics`.
>
> The key question for each proposal is: **does this refactoring interact with or complicate the ongoing namespace migration in Phase 6?**

---

### ✅ Safe to do now (no conflict with Phase 6)

These changes are self-contained within `main.cpp` or involve only additive changes to headers. They do not define new types, move symbols between namespaces, or add new translation units that Phase 6 would later need to re-namespace.

| # | Proposal | Why it's safe now |
|---|---|---|
| **2.1** | Remove duplicate log lines | Pure deletion inside an existing function. Zero namespace implications. |
| **2.5** | Fix indentation in `lmuffb_app_main` | Whitespace-only change inside an already-namespaced function. |
| **2.3** | Replace `static` locals with plain locals in `FFBThread` | Modifies existing variable declarations inside an already-namespaced function. No new symbols introduced. |
| **2.4** | Add a comment clarifying `g_running = false` shutdown sequencing | Comment-only. Zero risk. |
| **2.2** | Name magic number constants in an anonymous namespace in `main.cpp` | Adds `constexpr` constants inside `namespace { }` within an already-Unity-ready file. Fully invisible to other translation units. Phase 6 migrations touching `utils/` or `physics/` will not interact with these constants. |

**Recommendation:** Implement §2.1, §2.5, §2.4 in the current sprint as a quick housekeeping commit. §2.3 and §2.2 can follow immediately after.

---

### ⚠️ Safe now, but warrants care due to Phase 6 overlap

These changes touch files or symbols that **Phase 6 will later rename or reorganise**, so doing them now creates a slightly larger diff in a future Phase 6 PR. They are not blocked, but coordination is needed.

#### §4.2 — Expose upsample constants from `UpSampler.h`

`UpSampler.h` currently lives in `namespace LMUFFB`. Phase 6 does not appear to plan a `LMUFFB::FFB` sub-namespace for the upsampler at this time (the plan only lists `logging`, `utils`, `physics`, `gui`). Therefore adding `constexpr` members to `PolyphaseResampler` now is safe — the class stays in `namespace LMUFFB` for the foreseeable future.

**Recommendation:** Implement now. If `UpSampler.h` is eventually migrated to `LMUFFB::FFB`, the constants move with it trivially.

#### §3.1 — Extract `UpdateSessionLogging` free function

This is an anonymous-namespace free function inside `main.cpp`, which is already namespaced and whitelisted. Extracting it as a helper inside the same file's anonymous namespace has zero namespace implications. If it later moves to a dedicated header, that header will need its own namespace treatment — but that is a separate future decision.

**Recommendation:** Implement now as an in-file helper. Do **not** create a new header for it yet; wait until Phase 6 has settled the `LMUFFB::FFB` or similar sub-namespace before deciding where a `SessionManager.h` would live.

#### §3.2 — Extract `MaybeEmitHealthWarning` free function

Same reasoning as §3.1 — anonymous namespace, in-file helper. Safe to extract now.

**Recommendation:** Implement now.

---

### 🔴 Defer until Phase 6 is complete (or at least `physics/` and `utils/` are migrated)

These proposals introduce **new translation units or new class types** that would immediately need to be in the right sub-namespace. Doing them now in `namespace LMUFFB` root means doing the namespace migration work *twice* — once they will need to be moved when Phase 6 reaches them. They should wait until the sub-namespace plan for these modules is settled.

#### §3.3 — Move lost-frame detection into `FFBSafetyMonitor`

`FFBSafetyMonitor` is currently in `namespace LMUFFB`. Phase 6 is likely to move it to `namespace LMUFFB::FFB` (per the sub-namespace assignment in §6 of this document). Adding new state and methods to `FFBSafetyMonitor` now is low-risk today but creates a larger migration surface when Phase 6 reaches it.

**Recommendation:** Defer until the `ffb/` sub-namespace is decided and migrated. The lost-frame logic in `main.cpp` is well-localised and not urgent.

#### §4.1 — Introduce `PhysicsLoop` class

This is the highest-value proposal but also the most Phase-6-sensitive. `PhysicsLoop` is a new class that would hold `RateMonitor`, `ChannelMonitor`, and `PolyphaseResampler` instances — types from `LMUFFB::Logging` and (eventually) `LMUFFB::FFB`. Creating it now would mean:

1. Deciding its namespace today (either `LMUFFB` root as a temporary home, or `LMUFFB::FFB` which Phase 6 hasn't reached yet).
2. Immediately importing cross-namespace types (`LMUFFB::Logging::RateMonitor` etc.) inside the new class.
3. Potentially adding it to `UNITY_READY_MAIN`, which requires all its includes to be outside namespace blocks.

None of this is *impossible* now, but it requires running ahead of Phase 6's namespace design. If `PhysicsLoop` is created in `namespace LMUFFB` root, it will need a re-namespace pass when Phase 6 matures. If created directly in `namespace LMUFFB::FFB` (jumping ahead of Phase 6), it works but adds a new module to the Phase 6 tracking scope.

**Recommendation:** Defer `PhysicsLoop` extraction until Phase 6 has decided the final sub-namespace for `ffb/` modules. That decision will likely be driven by the `FFBEngine.h` migration, which is the most complex remaining Phase 6 item. At that point, `PhysicsLoop` can be created directly in the correct namespace.

**Exception:** The sub-changes within §4.1 that don't require a new class (e.g., pulling the phase accumulator logic into a cleaner helper function *within* `main.cpp`) can be done now as a preparatory step.

#### §5.2 — Introduce `ApplicationState` struct

`ApplicationState` aggregates `FFBEngine`, `SharedMemoryObjectOut`, and the global mutex. These types are currently in `namespace LMUFFB`. The struct itself would be straightforward to create now, but:

- `FFBEngine` is the largest, most coupled class in the codebase and the most significant Phase 6 migration target.
- Wrapping it in a struct before its own namespace migration is settled adds cognitive load to that future migration.
- The `#ifdef LMUFFB_UNIT_TEST` refactoring that `ApplicationState` enables (§5.1) is valuable, but not urgent — the current `extern` pattern works correctly.

**Recommendation:** Defer until `FFBEngine` has been migrated to its final sub-namespace (likely `LMUFFB::FFB`). At that point, `ApplicationState` can be created cleanly in `namespace LMUFFB` (root level is appropriate for a top-level aggregator).

---

### Summary Table

| # | Proposal | Timing | Rationale |
|---|---|---|---|
| 2.1 | Remove duplicate log lines | ✅ **Done** (v0.7.255) | Pure deletion, no namespace touch |
| 2.5 | Fix indentation | ✅ **Done** (v0.7.255) | Whitespace only |
| 2.4 | Comment `g_running` shutdown sequencing | ✅ **Done** (v0.7.255) | Comment only |
| 2.3 | Replace `static` locals with plain locals | ✅ **Done** (v0.7.255) | Scoped to existing namespaced function |
| 2.2 | Name magic number constants (anonymous ns) | ✅ **Done** (v0.7.255) | Fully internal to `main.cpp` |
| 4.2 | Expose upsample constants from `UpSampler.h` | ✅ **Now** | `UpSampler.h` not targeted by near-term Phase 6 |
| 3.1 | Extract `UpdateSessionLogging` (in-file helper) | ✅ **Now** | Anonymous namespace, no new header needed yet |
| 3.2 | Extract `MaybeEmitHealthWarning` (in-file helper) | ✅ **Now** | Same as §3.1 |
| 3.3 | Move lost-frame detection into `FFBSafetyMonitor` | 🔴 **After Phase 6** (`ffb/` migration) | Adds surface area to a soon-to-be-migrated class |
| 4.1 | Introduce `PhysicsLoop` class | 🔴 **After Phase 6** (`ffb/` namespace settled) | New type needs correct sub-namespace from the start |
| 5.1 | Audit `#ifdef LMUFFB_UNIT_TEST` pattern | ⚠️ **After §5.2** | Depends on `ApplicationState` |
| 5.2 | Introduce `ApplicationState` struct | 🔴 **After Phase 6** (`FFBEngine` migration) | Wraps the largest pending migration target |

---

*End of Report*
