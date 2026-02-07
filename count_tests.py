
import os
import subprocess
import glob
import re

def count_asserts(content):
    if isinstance(content, bytes):
        content = content.decode('utf-8', errors='ignore')
    return len(re.findall(r'ASSERT_|g_tests_passed\+\+', content))

def get_git_content(path):
    try:
        return subprocess.check_output(f'git show main:{path}', shell=True)
    except:
        return b""

print(f"{'File':<30} {'Main':<6} {'Head':<6} {'Diff':<6}")
print("-" * 50)

total_main = 0
total_head = 0

for filepath in glob.glob("tests/test_*.cpp"):
    filename = os.path.basename(filepath)
    rel_path = f"tests/{filename}"
    
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        head_content = f.read()
    
    head_count = count_asserts(head_content)
    
    main_content = get_git_content(rel_path)
    main_count = count_asserts(main_content)
    
    diff = head_count - main_count
    
    total_main += main_count
    total_head += head_count
    
    print(f"{filename:<30} {main_count:<6} {head_count:<6} {diff:<6}")

print("-" * 50)
print(f"{'TOTAL':<30} {total_main:<6} {total_head:<6} {total_head - total_main:<6}")
