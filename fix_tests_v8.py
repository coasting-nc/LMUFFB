import os
import re

test_files = [
    'tests/test_ffb_yaw_gyro.cpp',
    'tests/test_issue_241_yaw_kick_rectification.cpp',
    'tests/test_ffb_coordinates.cpp',
    'tests/test_ffb_road_texture.cpp',
    'tests/test_ffb_features.cpp',
    'tests/test_ffb_lockup_braking.cpp',
    'tests/test_ffb_slip_grip.cpp',
    'tests/test_ffb_internal.cpp',
    'tests/test_ffb_soft_lock.cpp',
    'tests/test_ffb_slope_fix.cpp',
    'tests/test_coverage_boost.cpp',
    'tests/test_upsampling.cpp',
    'tests/test_yaw_kick_derived.cpp',
    'tests/test_issue_374_repro.cpp',
    'tests/repro_issue_290.cpp',
    'tests/test_issue_322_yaw_kicks.cpp'
]

def fix_file(path):
    if not os.path.exists(path): return
    with open(path, 'r') as f:
        content = f.read()

    # Pattern: data.mSomething = val; engine.calculate_force(&data);
    # This is common in tests. We need a loop to settle.

    # We use a pattern that looks for data assignments followed by calculate_force.
    # We'll replace it with a loop that also increments mElapsedTime.

    def replacer(match):
        indent = match.group(1)
        assignment = match.group(2)
        call = match.group(3)
        # We need to increment time for is_new_frame to be true in each iteration
        return f"{indent}{assignment}\n{indent}for(int _i=0;_i<20;++_i) {{ data.mElapsedTime += 0.01; {call} }}"

    # Target: assignments to data members followed by calculate_force
    # Avoid replacing if it already has a loop or if it's the very first call in a test.
    new_content = re.sub(r'(\n\s+)(data\.[\w.\[\]]+\s*=\s*[^;]+;)\s+(engine\.calculate_force\(&data[^)]*\);)', replacer, content)

    if new_content != content:
        with open(path, 'w') as f:
            f.write(new_content)
        print(f"Fixed {path}")

for f in test_files:
    fix_file(f)
