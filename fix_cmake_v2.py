import os
filepath = 'tests/CMakeLists.txt'
with open(filepath, 'r') as f:
    content = f.read()

if 'test_issue_396.cpp' not in content:
    content = content.replace('test_math_utils_issue_393.cpp', 'test_math_utils_issue_393.cpp\n    test_issue_396.cpp')
with open(filepath, 'w') as f:
    f.write(content)
