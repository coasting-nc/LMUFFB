# Code Review - Iteration 1

## Changeset Overview
- **Issue**: #138 Add Game FFB (FFBTorque) to the Graphs
- **Version**: 0.7.61 -> 0.7.62
- **Core Files**: `FFBEngine.h`, `FFBEngine.cpp`, `AsyncLogger.h`, `GuiLayer_Common.cpp`
- **Tests**: `tests/test_ffb_diag_updates.cpp`

## Static Analysis & Logic Check

### 1. Data Propagation (`FFBEngine`)
- **FFBSnapshot**: Added `raw_shaft_torque` and `raw_gen_torque`. Correctly placed in Header C (Raw Telemetry).
- **calculate_force**:
    - Correctly captures `raw_torque_input` (whichever is selected) into `snap.steer_force`.
    - Correctly captures `data->mSteeringShaftTorque` into `snap.raw_shaft_torque`.
    - Correctly captures `genFFBTorque` into `snap.raw_gen_torque`.
- **Reliability**: Uses `(float)` cast for snapshot, which is consistent with existing fields. Thread safety is maintained as these are local variables until the snapshot is pushed under `m_debug_mutex`.

### 2. Telemetry Logging (`AsyncLogger`)
- **LogFrame**: Added `ffb_shaft_torque` and `ffb_gen_torque`.
- **CSV Format**:
    - Header updated: `FFBTotal,FFBBase,FFBShaftTorque,FFBGenTorque,FFBSoP...`
    - Row updated: matches header order.
- **FFBEngine Integration**: Logging block correctly populates the new fields.

### 3. GUI Visualization (`GuiLayer_Common`)
- **Rolling Buffers**: `plot_raw_shaft_torque` and `plot_raw_gen_torque` added. Correctly sized via `PLOT_BUFFER_SIZE`.
- **Data Collection**: Snapshots are correctly drained and added to buffers.
- **Rendering**:
    - Renamed "Steering Torque" to "Selected Torque" to avoid confusion.
    - Added "Shaft Torque (100Hz)" plot.
    - Added "Direct Torque (400Hz)" plot.
    - Used appropriate scales (-30 to 30 Nm) and tooltips.

### 4. Tests
- **TestFFBTorqueSnapshot**:
    - Verifies both torques are correctly passed to snapshot.
    - Verifies that switching source correctly updates `steer_force` while preserving raw channels.
    - Passes successfully.

### 5. Versioning
- `VERSION` file updated to `0.7.62`.
- `Version.h` and `res.rc` will be updated by CMake on next build.

## Concerns & Observations
- **Wait, does `genFFBTorque` need a sign flip?**
    - In `main.cpp`, `g_localData.generic.FFBTorque` is passed directly.
    - The engine has `m_invert_force` but that applies to the *final* output.
    - If `m_torque_source == 1`, `raw_torque_input` is `genFFBTorque`.
    - `mSteeringShaftTorque` in rF2/LMU usually matches the expected physics sign.
    - The user wants to *visualize* them. Seeing them side-by-side will help the user determine if one is inverted or much smaller.
- **CSV Column Order**: I inserted them between `FFBBase` and `FFBSoP`. This might break some custom log parsers if they rely on fixed indices, but this is a dev tool and we incremented the version.

## Conclusion: GREENLIGHT 🟢
The implementation is clean, follows the project's reliability standards, and fully addresses Issue #138.
