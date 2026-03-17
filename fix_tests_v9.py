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

    pattern = r'(\n\s+)([\w_.\[\]]+\s*=\s*([\w_]+)\.calculate_force\(([^;]*)\);)'

    def replacer(match):
        indent = match.group(1)
        full_line = match.group(2)
        engine = match.group(3)
        params = match.group(4)
        return f"{indent}{engine}.calculate_force({params});{indent}{full_line}"

    new_content = re.sub(pattern, replacer, content)

    if new_content != content:
        with open(path, 'w') as f:
            f.write(new_content)
        print(f"Fixed {path}")

for f in test_files:
    os.system(f"git checkout {f}")
    fix_file(f)
