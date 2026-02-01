TODO: mode this document to a folder for general reference document

# Coordinate System Reference Guide (v0.4.19)

**CRITICAL**: This document explains the coordinate system mismatch between rFactor 2/LMU and DirectInput that was fixed in v0.4.19. **Read this before modifying any FFB calculations involving lateral vectors.**

## Table of Contents
1. [The Fundamental Problem](#the-fundamental-problem)
2. [Coordinate System Definitions](#coordinate-system-definitions)
3. [Required Inversions](#required-inversions)
4. [Code Examples](#code-examples)
5. [Testing Strategy](#testing-strategy)
6. [Common Pitfalls](#common-pitfalls)

---

## The Fundamental Problem

The rFactor 2 / Le Mans Ultimate physics engine uses a **left-handed coordinate system** where **+X points to the driver's LEFT**. DirectInput steering wheels use the standard convention where **+Force means RIGHT**.

This creates a fundamental sign inversion for ALL lateral vectors (position, velocity, acceleration, force).

### Source of Truth

From `src/lmu_sm_interface/InternalsPlugin.hpp` lines 168-181:

```cpp
// Our world coordinate system is left-handed, with +y pointing up.
// The local vehicle coordinate system is as follows:
//   +x points out the left side of the car (from the driver's perspective)
//   +y points out the roof
//   +z points out the back of the car
// Rotations are as follows:
//   +x pitches up
//   +y yaws to the right
//   +z rolls to the right
```

### DirectInput Convention

- **Negative (-)**: Turn LEFT (Counter-Clockwise)
- **Positive (+)**: Turn RIGHT (Clockwise)

---

## Coordinate System Definitions

### Game Engine (rFactor 2 / LMU)

| Axis | Positive Direction | Example |
|------|-------------------|---------|
| **+X** | Left (driver's perspective) | Sliding left = +X velocity |
| **+Y** | Up (roof) | Jumping = +Y velocity |
| **+Z** | Back (rear bumper) | Reversing = +Z velocity |

### DirectInput (Steering Wheel)

| Value | Direction | Torque Effect |
|-------|-----------|---------------|
| **Negative (-)** | Left | Pull wheel left |
| **Positive (+)** | Right | Pull wheel right |

### The Conflict

| Physical Event | Game Data | Desired Wheel Feel | Required Inversion |
|----------------|-----------|-------------------|-------------------|
| Right turn (body feels left force) | `mLocalAccel.x = +9.81` | Pull LEFT (heavy steering) | **YES** - Invert sign |
| Rear slides left (oversteer) | `mLateralPatchVel = +5.0` | Counter-steer LEFT | **YES** - Invert sign |
| Sliding left | `mLateralPatchVel = +5.0` | Friction pushes RIGHT | **NO** - Keep sign |

---

## Required Inversions

### 1. Seat of Pants (SoP) - Lateral G

**Location**: `FFBEngine.h` line ~571

**Physics**: In a right turn, the body feels centrifugal force to the LEFT. The steering should feel heavy (pull LEFT) to simulate load transfer.

**Code**:
```cpp
// WRONG (pre-v0.4.19):
double lat_g = raw_g / 9.81;

// CORRECT (v0.4.19+):
double lat_g = -(raw_g / 9.81);  // INVERT to match DirectInput
```

**Why**: 
- Right turn → Body accelerates LEFT → `mLocalAccel.x = +9.81`
- We want: Wheel pulls LEFT (negative force)
- Without inversion: `+9.81 / 9.81 = +1.0` → Pulls RIGHT ❌
- With inversion: `-(+9.81 / 9.81) = -1.0` → Pulls LEFT ✓

---

### 2. Rear Aligning Torque - Counter-Steering

**Location**: `FFBEngine.h` line ~666

**Physics**: When the rear slides, tires generate lateral force that should provide counter-steering cues.

**Code**:
```cpp
// WRONG (pre-v0.4.19):
double rear_torque = calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;

// CORRECT (v0.4.19+):
double rear_torque = -calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;  // INVERT
```

**Why**:
- Rear slides LEFT → Slip angle is POSITIVE → Lateral force is POSITIVE
- We want: Counter-steer LEFT (negative force)
- Without inversion: Positive force → Pulls RIGHT → **CATASTROPHIC POSITIVE FEEDBACK LOOP** ❌
- With inversion: Negative force → Pulls LEFT → Corrects the slide ✓

**This was the root cause of the user-reported bug**: "Slide rumble throws the wheel in the direction I am turning."

---

### 3. Scrub Drag - Friction Direction

**Location**: `FFBEngine.h` line ~840

**Physics**: Friction opposes motion. If sliding left, friction pushes right.

**Code**:
```cpp
// WRONG (pre-v0.4.19):
double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;  // If left, push left (WRONG!)

// CORRECT (v0.4.19+):
double drag_dir = (avg_lat_vel > 0.0) ? 1.0 : -1.0;  // If left, push right (opposes motion)
```

**Why**:
- Sliding LEFT → `mLateralPatchVel = +5.0`
- Friction opposes motion → Should push RIGHT
- Without fix: Pushes LEFT → Accelerates the slide (negative damping) ❌
- With fix: Pushes RIGHT → Resists the slide ✓

---

## Code Examples

### Example 1: Adding a New Lateral Effect

```cpp
// ❌ WRONG - Direct use of game data
double new_effect = data->mLocalAccel.x * some_coefficient;

// ✓ CORRECT - Invert for DirectInput
double new_effect = -(data->mLocalAccel.x) * some_coefficient;
```

### Example 2: Checking Your Work

Ask yourself these questions:

1. **What is the physical event?** (e.g., "Right turn")
2. **What does the game report?** (e.g., `mLocalAccel.x = +9.81`)
3. **What should the wheel feel?** (e.g., "Pull LEFT to simulate heavy steering")
4. **What sign does DirectInput need?** (e.g., "Negative for LEFT")
5. **Do I need to invert?** (e.g., "YES - game says +9.81, I need negative")

### Example 3: Friction/Damping Effects

Friction and damping effects that **oppose motion** may NOT need inversion:

```cpp
// Scrub drag: Friction opposes the slide direction
// If sliding left (+vel), friction pushes right (+force)
// NO INVERSION needed - the physics naturally provides the correct sign
double drag_dir = (avg_lat_vel > 0.0) ? 1.0 : -1.0;
```

---

## Testing Strategy

### Unit Tests

Every coordinate-sensitive effect MUST have regression tests in `tests/test_ffb_engine.cpp`:

```cpp
void test_coordinate_[effect_name]() {
    // Test Case 1: Positive game input (LEFT)
    data.mLocalAccel.x = 9.81;  // Left acceleration
    double force = engine.calculate_force(&data);
    ASSERT_TRUE(force < 0.0);  // Should pull LEFT (negative)
    
    // Test Case 2: Negative game input (RIGHT)
    data.mLocalAccel.x = -9.81;  // Right acceleration
    force = engine.calculate_force(&data);
    ASSERT_TRUE(force > 0.0);  // Should pull RIGHT (positive)
}
```

### Manual Testing

1. **Right Turn Test**:
   - Drive in a steady right turn
   - Wheel should feel HEAVY (pulling left)
   - If wheel feels LIGHT, SoP is inverted ❌

2. **Oversteer Test**:
   - Induce oversteer (rear slides out)
   - Wheel should provide counter-steering cue
   - If wheel pulls INTO the slide, rear torque is inverted ❌

3. **Drift Test**:
   - Slide sideways at constant angle
   - Wheel should feel resistance (friction)
   - If wheel feels assisted (negative damping), scrub drag is inverted ❌

---

## Common Pitfalls

### Pitfall 1: "The Math Looks Right"

```cpp
// This looks mathematically correct:
double sop_force = lateral_g * coefficient;

// But it's WRONG for DirectInput!
// You must invert:
double sop_force = -lateral_g * coefficient;
```

**Lesson**: Trust the coordinate system, not your intuition.

### Pitfall 2: "It Works in One Direction"

If an effect only feels wrong in one direction (e.g., only in left turns), you likely have a sign error.

### Pitfall 3: "Abs() Hides the Problem"

```cpp
// Using abs() can mask coordinate issues:
double slip_angle = std::atan2(std::abs(mLateralPatchVel), longitudinal_vel);

// This loses directional information!
// Consider whether you need the sign for your effect.
```

### Pitfall 4: "Positive Feedback Loops"

If an effect makes the car HARDER to control instead of easier, you likely have an inverted sign creating positive feedback.

**Example**: Rear aligning torque pulling INTO the slide instead of providing counter-steering.

---

## Checklist for New Effects

Before adding any new lateral effect, complete this checklist:

- [ ] I have identified the game coordinate system for all input variables
- [ ] I have determined the desired DirectInput output sign
- [ ] I have applied inversions where needed
- [ ] I have added unit tests for both left and right scenarios
- [ ] I have manually tested the effect in-game
- [ ] I have documented the coordinate system logic in code comments
- [ ] I have verified no positive feedback loops exist

---

## References

- **Bug Report**: `docs/bug_reports/wrong rf2 coordinates use.md`
- **Source Code**: `FFBEngine.h` lines 571, 666, 840
- **API Documentation**: `src/lmu_sm_interface/InternalsPlugin.hpp` lines 168-181
- **Test Suite**: `tests/test_ffb_engine.cpp` - Search for "v0.4.19" or "coordinate"

---

## Version History

- **v0.4.19** (2025-12-16): Initial documentation after fixing three critical coordinate inversions
- **Future**: This document should be updated whenever new lateral effects are added

---

**⚠️ WARNING**: Ignoring this coordinate system mismatch can create positive feedback loops that make the car uncontrollable. Always verify sign conventions when working with lateral vectors.
