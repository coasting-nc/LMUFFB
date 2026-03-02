The proposed patch for Issue #218 correctly implements the calculation of steering telemetry in degrees and includes the one-time warning logic for invalid range data. However, it is **not commit-ready** due to critical thread-safety violations and missing reset triggers.

### User's Goal
The goal is to display real-time steering range and angle (in degrees) in the GUI and issue a one-time console warning if the game reports an invalid steering range ($\le 0$).

### Evaluation of the Solution

#### Core Functionality
- **Telemetry Calculation:** The conversion from radians to degrees and the calculation of the current angle based on normalized steering input are mathematically correct and centralized in the FFB loop.
- **Diagnostic Warning:** The one-time warning logic is correctly implemented, ensuring that the console is not flooded with redundant messages during gameplay.

#### Safety & Side Effects
- **Thread Safety (BLOCKING):** The patch violates the project's core reliability standards by allowing the GUI thread (`GuiLayer_Common.cpp`) to directly access internal `FFBEngine` members (`m_steering_angle_deg`, `m_steering_range_deg`) without a mutex or atomic protection. These members are modified at 400Hz in the FFB thread, leading to potential data races or tearing.
- **State Management:** The diagnostic flag `m_warned_invalid_range` is never reset. This means that if a user switches from a car with broken telemetry to one that works, the engine will not re-evaluate the warning if the telemetry breaks again later in the session.

#### Completeness
- **Metadata:** The `VERSION` and `CHANGELOG_DEV.md` are correctly updated to `0.7.112`.
- **Testing:** The new tests in `test_issue_218_steering.cpp` are high quality and cover the core requirements well.

### Merge Assessment (Blocking vs. Non-Blocking)

**Blocking:**
1.  **Refactor for Thread Safety:** Telemetry data must be passed to the GUI via the existing `FFBSnapshot` system or protected by `g_engine_mutex`. Direct access to non-atomic engine members from the GUI thread is prohibited.
2.  **Add Reset Triggers:** The diagnostic flag should be reset when the vehicle changes or when the user manually resets normalization to ensure warnings are issued for each session/car.

**Nitpicks:**
1.  **Naming:** The member `m_steering_angle_deg` could be more descriptive (e.g., `m_current_steering_angle_deg`) to distinguish it from the static range.

### Final Rating: #Partially Correct#
