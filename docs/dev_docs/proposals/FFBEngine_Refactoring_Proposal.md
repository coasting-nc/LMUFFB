# Proposal: Refactoring `FFBEngine.h`

## Overview
The current `FFBEngine.h` file is approximately 2,100 lines long and handles a wide range of responsibilities, including telemetry processing, physics modeling, signal conditioning, mathematical utilities, and vehicle-specific logic. 

To improve maintainability, testability, and build times, I propose splitting this functionality into several logical modules and moving implementation details out of the header file.

## Current Responsibilities
1.  **Data Structures**: `FFBSnapshot`, `FFBCalculationContext`, `ChannelStats`.
2.  **Mathematical Utilities**: Signal filters (Notch), smoothing functions, derivative calculations (Savitzky-Golay).
3.  **Vehicle Intelligence**: Class parsing, lookup tables for static loads.
4.  **Physics Modeling**: Slip angle calculations, grip estimation, kinematic load estimation.
5.  **Effect Synthesis**: Logic for SoP (Sense of Poster), ABS, lockup vibrations, road texture, etc.
6.  **Core Orchestration**: The `calculate_force` method that ties everything together.

---

## Proposed Refactoring Steps

### 1. Extract Core Types (`FFBTypes.h`)
Move data-only structures and enums to a dedicated header. This allows other components to reference these types without pulling in the entire engine.
- `FFBSnapshot`
- `FFBCalculationContext`
- `ChannelStats`
- `GripResult`
- `ParsedVehicleClass` (Enum)

### 2. Signal Processing Library (`SignalUtils.h / .cpp`)
Move general-purpose signal processing and math functions to a utility module.
- `class BiquadNotch`
- `apply_adaptive_smoothing()`
- `apply_slew_limiter()`
- `calculate_sg_derivative()`
- `inverse_lerp()`
- `smoothstep()`

### 3. Physics & Telemetry Model (`FFBPhysics.h / .cpp`)
Encapsulate the "Internal Model" of the car's physics. This module would be responsible for translating raw telemetry into high-level physics states (grip, slip, load).
- `calculate_slip_angle()`
- `calculate_grip()`
- `calculate_kinematic_load()`
- `calculate_wheel_slip_ratio()`
- `approximate_load()`

### 4. Vehicle & Content Library (`VehicleLibrary.h / .cpp`)
Separate the logic that deals with specific game content and vehicle categorization.
- `ParseVehicleClass()`
- `GetDefaultLoadForClass()`
- `VehicleClassToString()`
- Load reference initialization logic.

### 5. Effect Modules (`FFBEffects.h / .cpp`)
Break down the individual effect calculations. These could either be static functions in a namespace or separate strategy classes.
- `calculate_sop_lateral()`
- `calculate_abs_pulse()`
- `calculate_lockup_vibration()`
- `calculate_wheel_spin()`
- `calculate_suspension_bottoming()`
- etc.

### 6. Split Header/Implementation (`FFBEngine.h` & `FFBEngine.cpp`)
Finally, convert `FFBEngine.h` into a standard C++ class declaration and move the remaining orchestration logic to `FFBEngine.cpp`.

---

## Benefits
- **Reduced Build Times**: Changes to a specific effect or physics calculation will only require recompiling one small `.cpp` file instead of everything that includes `FFBEngine.h`.
- **Improved Testability**: We can write unit tests for `FFBPhysics` or `SignalUtils` in isolation without needing a full `FFBEngine` instance.
- **Readability**: Navigating a 200-line header is significantly easier than a 2000-line one.
- **Separation of Concerns**: Clearly distinguishes between *how* we get physics data and *what* we do with it to generate forces.

## Next Steps
1.  **Phase A**: Extract `FFBTypes.h` and `SignalUtils.h`.
2.  **Phase B**: Create `FFBPhysics.h/cpp` and move physics helpers.
3.  **Phase C**: Create `VehicleLibrary.h/cpp`.
4.  **Phase D**: Move implementation of `FFBEngine` methods to a new `FFBEngine.cpp`.
5.  **Phase E**: Decompose `calculate_force` by moving effect logic to `FFBEffects.h/cpp`.
