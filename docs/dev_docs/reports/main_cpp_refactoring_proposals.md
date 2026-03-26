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

### 2.1 Duplicate Log Lines

**Lines 327–328 and 332–333** call `Logger::Get().Log(...)` twice with the same string.

```cpp
// Issue: duplicated log call
Logger::Get().Log("Failed to initialize GUI.");
Logger::Get().Log("Failed to initialize GUI."); // <-- redundant

Logger::Get().Log("Running in HEADLESS mode.");
Logger::Get().Log("Running in HEADLESS mode."); // <-- redundant
```

**Proposal:** Remove the duplicate calls. Also consider using `LogFile` (which writes to disk) instead of `Log` for failure states so the message survives a crash.

---

### 2.2 Magic Numbers

Several numeric literals appear inline without named constants:

| Location | Value | Meaning |
|---|---|---|
| Line 90 | `1000` | FFB loop target frequency (Hz) |
| Line 98 | `2` | Upsampling `M` coefficient |
| Line 101 | `5` | Upsampling `L` coefficient |
| Line 119 | `100` | Staleness threshold (ms) |
| Line 146 | `104` | Max vehicle index (game-imposed limit) |
| Line 204 | `0.0025` | Physics timestep (s) |
| Line 214 | `0.0025` | Safety slew physics timestep (s) |
| Line 239 | `60` | Health warning cooldown (seconds) |
| Line 358 | `16` | GUI loop period (ms, ≈ 60 Hz) |

**Proposal:** Define named constants in an anonymous namespace at the top of the file, or—more ideally—expose the sampling/upsampling constants from `UpSampler.h` and `FFBSafetyMonitor.h` where they conceptually belong.

```cpp
namespace {
    constexpr int    kFFBTargetHz         = 1000;
    constexpr int    kPhysicsUpsampleL    = 5;    // Upsample ratio numerator
    constexpr int    kPhysicsUpsampleM    = 2;    // Upsample ratio denominator
    constexpr double kPhysicsTimestepSec  = 0.0025; // 1/400 Hz
    constexpr int    kStaleThresholdMs    = 100;
    constexpr int    kMaxVehicleIndex     = 104;
    constexpr int    kHealthWarnCooldownS = 60;
    constexpr int    kGuiPeriodMs         = 16;   // ~60 Hz
} // anonymous namespace
```

> [!NOTE]
> `kPhysicsTimestepSec` (1/400 Hz) and `kPhysicsUpsampleL/M` are directly related to `PolyphaseResampler`'s design and should ideally be declared in `UpSampler.h` as `constexpr` members so they're co-located with the algorithm they describe.

---

### 2.3 `static` Locals Inside a Hot Loop

`FFBThread` uses several `static` local variables:

```cpp
static bool was_driving = false;
static long last_session = -1;
static double last_telem_et = -1.0;
static auto lastWarningTime = TimeUtils::GetTime();
```

**Problems:**
1. `static` locals are zero-initialized once and retained across loop iterations, which is the correct intent — but this is semantically fragile, since the thread is expected to run only once. If the thread is ever restarted (e.g., reconnect flow, test teardown), the statics retain stale values.
2. `static` locals inside non-trivially-destructed lambdas or thread functions are a common source of **test flakiness** — they bleed state between test cases.
3. `static` variables communicate *intent* poorly; a reader must figure out they are serving as "last known state" rather than file-scope or class-scope singletons.

**Proposal:** Promote all loop-state variables to the top of `FFBThread` as plain (non-static) local variables. Since `FFBThread` is not restarted mid-run, this is zero-risk functionally, and it makes state scoping explicit.

```cpp
// Before (static locals — fragile in tests)
static bool was_driving = false;
static long last_session = -1;

// After (ordinary locals — reset on every thread launch)
bool was_driving = false;
long last_session = -1;
```

---

### 2.4 `g_running = false` After `ffb_thread.joinable()` Check

```cpp
if (ffb_thread.joinable()) {
    Logger::Get().LogFile("Stopping FFB Thread...");
    g_running = false; // <-- already set by GUI loop exit condition?
    ffb_thread.join();
}
```

`g_running` is the condition that terminates the `while (g_running)` GUI loop (line 348), so it is `false` at this point by the time we reach this code. Setting it again is redundant but harmless. Add a comment explaining the shutdown sequencing to avoid confusion for future readers.

---

### 2.5 Indentation Error in `lmuffb_app_main`

Lines 303–306 are not indented to match the enclosing `try` block (they should be at the same depth as the `#ifdef _WIN32` block above):

```cpp
int lmuffb_app_main(int argc, char* argv[]) noexcept {
    try {
#ifdef _WIN32
        timeBeginPeriod(1);
#else
        ...
#endif

    bool headless = false;   // <-- missing one level of indent
    for (int i = 1; i < argc; ++i) {
```

**Proposal:** Re-indent lines 303–383 consistently inside the `try` body.

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

| Priority | Proposal | Effort | Risk |
|---|---|---|---|
| 🟢 **1** | Fix duplicate log lines (§2.1) | Trivial | Zero |
| 🟢 **2** | Fix indentation in `lmuffb_app_main` (§2.5) | Trivial | Zero |
| 🟢 **3** | Replace `static` locals with plain locals (§2.3) | Low | Low |
| 🟢 **4** | Name the magic number constants (§2.2) | Low | Low |
| 🟡 **5** | Expose upsample constants from `UpSampler.h` (§4.2) | Low–Medium | Low |
| 🟡 **6** | Extract `UpdateSessionLogging` helper (§3.1) | Medium | Low |
| 🟡 **7** | Extract `MaybeEmitHealthWarning` helper (§3.2) | Medium | Low |
| 🟡 **8** | Move lost-frame detection into `FFBSafetyMonitor` (§3.3) | Medium | Medium |
| 🔴 **9** | Introduce `PhysicsLoop` class (§4.1) | High | Medium |
| 🔴 **10** | Introduce `ApplicationState` struct (§5.2) | High | Medium |

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

*End of Report*
