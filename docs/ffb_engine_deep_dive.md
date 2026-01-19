# FFB Engine Deep Dive

## Overview

This document provides a comprehensive technical explanation of the LMUFFB Force Feedback Engine, covering architecture, data paths, safety mechanisms, and implementation details.

## Architecture Overview

### Core Components

- **FFBEngine Class**: Header-only real-time engine running at 400Hz
- **FFBSnapshot**: Ring buffer for GUI synchronization
- **TelemInfoV01/TelemWheelV01**: LMU 1.2 telemetry structures
- **Config System**: Persistent settings with safe defaults

### Real-time Loop (400Hz)

```cpp
double calculate_force(const TelemInfoV01* data) {
    // 1. Sanity checks (delta time, etc.)
    // 2. Extract telemetry (steering torque, wheel data)
    // 3. Apply signal processing (smoothing, filtering)
    // 4. Calculate physics effects (grip, slip, textures)
    // 5. Sum and clamp all forces
    // 6. Populate snapshot for GUI
    return final_force;
}
```

## Data Flow

### Input Pipeline

1. **Game Telemetry** → LMU Shared Memory → `TelemInfoV01`
2. **Signal Conditioning** → Smoothing, clamping, fallbacks
3. **Physics Calculations** → Grip, slip angles, load factors
4. **Effect Summation** → Combine all FFB components
5. **Output** → Hardware drivers (vJoy, DirectInput, etc.)

### GUI Synchronization

- **Producer**: Real-time thread writes to `FFBSnapshot` ring buffer
- **Consumer**: GUI thread reads snapshots at 60Hz
- **Memory Barriers**: Ensure thread-safe access without locks

## Safety Mechanisms

### Input Validation

- **Delta Time**: Clamped to prevent division by zero
- **Load Factors**: Hysteresis for missing telemetry
- **Slip Angles**: Singularity protection at low speeds
- **Force Clamping**: Hardware damage prevention

### Fallback Systems

- **Kinematic Load**: Physics-based load reconstruction
- **Friction Circle**: Grip approximation from slip data
- **Signal Defaults**: Graceful degradation on missing data

## New Telemetry Mappings (v0.7.0+)

### Axle 3rd Deflection

**Purpose**: Enhanced suspension feedback using 3rd spring deflection data.

**Signals**:
- `mFront3rdDeflection`: 0-1 range, front axle
- `mRear3rdDeflection`: 0-1 range, rear axle

**Mapping**:
```cpp
// FL/FR get front axle deflection added
mWheel[0].mSuspensionDeflection += mFront3rdDeflection;
mWheel[1].mSuspensionDeflection += mFront3rdDeflection;

// RL/RR get rear axle deflection added
mWheel[2].mSuspensionDeflection += mRear3rdDeflection;
mWheel[3].mSuspensionDeflection += mRear3rdDeflection;
```

**Result**: Each wheel's suspension deflection ranges from 0-2 instead of 0-1.

### Suspension Force Enhancements

**Signals**:
- `mRideHeight`: Per-wheel chassis height
- `mDrag`: Global aerodynamic drag
- `mFrontDownforce`/`mRearDownforce`: Wing downforce

**Mapping**:
```cpp
// Per-wheel ride height contribution
mSuspForce += rideHeightFunction(mRideHeight);

// Global drag distributed
mSuspForce += mDrag / 4;

// Axle-specific downforce
if (isFrontWheel) mSuspForce += mFrontDownforce / 2;
else mSuspForce += mRearDownforce / 2;
```

### Tire Condition Effects

**Wear → Grip**:
```cpp
mGripFract *= (1.0f - mWear);  // Wear reduces available grip
```

**Flat → Load**:
```cpp
if (mFlat) mTireLoad *= 0.2f;  // Flat tires carry 20% normal load
```

## Implementation Notes

### Phase Accumulation for Oscillators

All vibration effects use phase accumulation instead of time-based sine waves:

```cpp
// CORRECT: Deterministic phase accumulation
m_phase += frequency * deltaTime;
output = sin(m_phase);

// WRONG: Time-based (drifts with frame variance)
output = sin(elapsedTime * frequency);
```

### Memory Management

- **No heap allocation** in real-time path
- **Fixed-size buffers** for all data structures
- **Stack-only operations** in calculate_force()

### Threading Model

- **Real-time thread**: 400Hz physics + FFB calculation
- **GUI thread**: 60Hz visualization + snapshot consumption
- **Config thread**: Asynchronous settings updates

## Testing Strategy

### Unit Tests
- Individual effect calculations
- Signal processing pipelines
- Edge cases and fallbacks

### Integration Tests
- Full FFB pipeline with telemetry
- GUI synchronization
- Performance benchmarks

### Validation
- Hardware compatibility testing
- Regression prevention
- User acceptance testing