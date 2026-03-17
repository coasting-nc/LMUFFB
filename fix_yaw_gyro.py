import os

filepath = 'tests/test_ffb_yaw_gyro.cpp'
with open(filepath, 'r') as f:
    content = f.read()

# Fix test_sop_yaw_kick
# We need Call 1: seed 0. Call 2: set target. Call 3: get value.
content = content.replace(
    '    // Seeding frame\n    data.mLocalRot.y = 0.0;\n    engine.calculate_force(&data);\n    \n    // Test frame: rate moves to 1.0 * dt\n    data.mLocalRot.y = 1.0 * 0.0025; \n    \n    // Ensure no other inputs',
    '    // Seeding frame\n    data.mLocalRot.y = 0.0;\n    data.mElapsedTime = 0.0;\n    engine.calculate_force(&data);\n    \n    // Establish target\n    data.mLocalRot.y = 1.0 * 0.01; \n    data.mElapsedTime = 0.01;\n    engine.calculate_force(&data);\n\n    // Get value (Interpolation completes)\n    data.mElapsedTime = 0.02;\n    // Ensure no other inputs'
)

# Fix test_gyro_damping
content = content.replace(
    '    // Frame 1: Steering at 0.0\n    data.mUnfilteredSteering = 0.0f;\n    engine.calculate_force(&data);\n    \n    // Frame 2: Steering moves to 0.1 (rapid movement to the right)\n    data.mUnfilteredSteering = 0.1f;\n    double force = engine.calculate_force(&data);',
    '    // Frame 1: Steering at 0.0\n    data.mUnfilteredSteering = 0.0f;\n    data.mElapsedTime = 0.0;\n    engine.calculate_force(&data);\n    \n    // Establish target\n    data.mUnfilteredSteering = 0.1f;\n    data.mElapsedTime = 0.01;\n    engine.calculate_force(&data);\n\n    // Frame 2: Step time to let interpolation deliver value\n    data.mElapsedTime = 0.02;\n    double force = engine.calculate_force(&data);'
)

# Fix test_gyro_damping reverse direction
content = content.replace(
    '    // Frame 3: Steering moves back from 0.1 to 0.0 (negative velocity)\n    data.mUnfilteredSteering = 0.0f;\n    engine.calculate_force(&data);',
    '    // Frame 3: Steering moves back from 0.1 to 0.0 (negative velocity)\n    data.mUnfilteredSteering = 0.0f;\n    data.mElapsedTime = 0.03;\n    engine.calculate_force(&data);\n    data.mElapsedTime = 0.04;\n    engine.calculate_force(&data);'
)

# Fix test_gyro_damping speed scaling
content = content.replace(
    '    data.mUnfilteredSteering = 0.0f;\n    engine.calculate_force(&data);\n    \n    data.mUnfilteredSteering = 0.1f;\n    engine.calculate_force(&data);',
    '    data.mUnfilteredSteering = 0.0f;\n    data.mElapsedTime = 0.05;\n    engine.calculate_force(&data);\n    \n    data.mUnfilteredSteering = 0.1f;\n    data.mElapsedTime = 0.06;\n    engine.calculate_force(&data);\n    data.mElapsedTime = 0.07;\n    engine.calculate_force(&data);'
)

# Fix test_yaw_accel_smoothing
content = content.replace(
    '    // Seeding\n    data.mLocalRot.y = 0.0;\n    engine.calculate_force(&data);\n    \n    // Spike: rate moves to 10.0 * dt\n    data.mLocalRot.y = 10.0 * 0.0025;\n    \n    double force_frame1 = engine.calculate_force(&data);',
    '    // Seeding\n    data.mLocalRot.y = 0.0;\n    data.mElapsedTime = 0.0;\n    engine.calculate_force(&data);\n    \n    // Establish target\n    data.mLocalRot.y = 10.0 * 0.01;\n    data.mElapsedTime = 0.01;\n    engine.calculate_force(&data);\n\n    // Frame 1: Interpolation delivers value\n    data.mElapsedTime = 0.02;\n    double force_frame1 = engine.calculate_force(&data);'
)
content = content.replace(
    '    data.mLocalRot.y += 10.0 * 0.0025;\n    double force_frame2 = engine.calculate_force(&data);',
    '    data.mLocalRot.y += 10.0 * 0.01;\n    data.mElapsedTime = 0.03;\n    engine.calculate_force(&data);\n    data.mElapsedTime = 0.04;\n    double force_frame2 = engine.calculate_force(&data);'
)

# Fix test_sop_yaw_kick_direction
content = content.replace(
    '    data.mLocalRot.y = 0.0;\n    engine.calculate_force(&data); // Seed\n    data.mLocalRot.y = 5.0 * 0.0025; \n    data.mLocalVel.z = 20.0; // v0.4.42: Ensure speed > 5 m/s for Yaw Kick \n    \n    double force = engine.calculate_force(&data);',
    '    data.mLocalRot.y = 0.0;\n    data.mElapsedTime = 0.0;\n    engine.calculate_force(&data); // Seed\n    data.mLocalRot.y = 5.0 * 0.01; \n    data.mElapsedTime = 0.01;\n    engine.calculate_force(&data);\n    data.mElapsedTime = 0.02;\n    data.mLocalVel.z = 20.0;\n    double force = engine.calculate_force(&data);'
)

with open(filepath, 'w') as f:
    f.write(content)
