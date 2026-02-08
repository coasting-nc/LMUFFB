"""
LMUFFB Project Context Generator

This script collects all source code files and documentation from the LMUFFB project
into a single consolidated markdown file. This file can be used as context when
querying Large Language Models (LLMs) about the project.

The script:
- Recursively walks through the project directory
- Collects all code files (.cpp, .h, .py, etc.) and documentation (.md, .txt)
- Formats each file in markdown code blocks with appropriate syntax highlighting
- Outputs everything to docs/dev_docs/FULL_PROJECT_CONTEXT.md

This is useful for providing complete project context to AI assistants or for
documentation purposes.

Usage:
    python scripts/create_context.py

Output:
    docs/dev_docs/FULL_PROJECT_CONTEXT.md
"""

import os
import re
import argparse
import sys
import subprocess

OUTPUT_FILE = "FULL_PROJECT_CONTEXT.md"

# Extensions to include
CODE_EXTENSIONS = {
    '.cpp', '.h', '.c', '.hpp', 
    '.iss', '.py'
}

MAKE_EXTENSIONS = {
    '.cmake'
}

MAKE_FILES = {
    'CMakeLists.txt', 'Makefile'
}

DOC_EXTENSIONS = {
    '.md', '.txt', '.log'
}

# Directories to exclude (by base name)
EXCLUDE_DIRS = {
    'build', 'build_test', 'build_tests', 'build-tests', '.git', 'python_prototype', 'vendor', '__pycache__', '.vscode', '.specstory', '.pytest_cache'
}

# Full relative paths to exclude (normalized with forward slashes)
EXCLUDE_DIR_PATHS = {
    'docs/dev_docs/done implementations reports',
    'docs/dev_docs/implementation plan reviews',
    'docs/dev_docs/implementation_plans/completed',
    'tools',
    'installer',
    'scripts'
}

# Files to exclude
EXCLUDE_FILES = {
    OUTPUT_FILE, 'LICENSE', 'compile_commands.json', 'TODO.md', 'prompts_for_coding_agents.md', 'create_context.py',
    'LEGACY_JULES_ONLY_AGENTS_MEMORY.md', 'LEGACY_JULES_ONLY_AGENTS.md', 'USER_CHANGELOG.md'
}

# Specific test files to serve as examples when --test-examples-only is used
EXAMPLE_TEST_FILES = {
    'tests/main_test_runner.cpp',
    'tests/test_ffb_common.cpp',
    'tests/test_ffb_common.h'
}

def is_code_file(filename):
    """Check if a file is considered a code file."""
    return any(filename.endswith(ext) or filename == ext for ext in CODE_EXTENSIONS)

def is_doc_file(filename):
    """Check if a file is considered a documentation file."""
    return any(filename.endswith(ext) for ext in DOC_EXTENSIONS)

def is_make_file(filename):
    """Check if a file is a makefile or related config."""
    return filename in MAKE_FILES or any(filename.endswith(ext) for ext in MAKE_EXTENSIONS)

def is_test_file(relpath, filename):
    """Check if a file is considered a test file."""
    relpath_normalized = relpath.replace('\\', '/')
    parts = relpath_normalized.split('/')
    if 'tests' in parts:
        return True
    if filename.startswith('test_') or '_test.' in filename:
        return True
    return False

def is_main_code(relpath_normalized):
    """Check if a file is part of the main application code (src)."""
    return relpath_normalized.startswith('src/')

def is_ignored(relpath, root_dir):
    """Check if a file is ignored by git."""
    try:
        # Use normalized relative path for git check-ignore
        relpath_git = relpath.replace('\\', '/')
        result = subprocess.run(['git', 'check-ignore', '-q', relpath_git], capture_output=False, cwd=root_dir)
        return result.returncode == 0
    except:
        return False

def parse_args(args=None):
    parser = argparse.ArgumentParser(description="LMUFFB Project Context Generator")
    
    # Tests options
    test_group = parser.add_mutually_exclusive_group()
    test_group.add_argument("--include-tests", action="store_true", dest="include_tests", help="Include test code")
    test_group.add_argument("--exclude-tests", action="store_false", dest="include_tests", help="Exclude test code")
    
    parser.add_argument("--test-examples-only", action="store_true", help="Include only 1-2 example test files")

    # Non-code options
    doc_group = parser.add_mutually_exclusive_group()
    doc_group.add_argument("--include-non-code", action="store_true", dest="include_non_code", help="Include non-code files (.md, .txt, .log)")
    doc_group.add_argument("--exclude-non-code", action="store_false", dest="include_non_code", help="Exclude non-code files")
    
    # Main code options
    main_group = parser.add_mutually_exclusive_group()
    main_group.add_argument("--include-main-code", action="store_true", dest="include_main_code", help="Include main source code (src/)")
    main_group.add_argument("--exclude-main-code", action="store_false", dest="include_main_code", help="Exclude main source code")
    
    # Makefile options
    make_group = parser.add_mutually_exclusive_group()
    make_group.add_argument("--include-makefiles", action="store_true", dest="include_makefiles", help="Include makefiles (CMakeLists.txt, etc.)")
    make_group.add_argument("--exclude-makefiles", action="store_false", dest="include_makefiles", help="Exclude makefiles")
    
    # Set defaults if not specified by the injection logic in main
    parser.set_defaults(include_tests=True, include_non_code=True, include_main_code=True, include_makefiles=True, test_examples_only=False)
    
    return parser.parse_args(args)

def main():
    """
    Main function that generates the consolidated project context file.
    """
    # Configuration of defaults - change these to easily update default behavior
    DEFAULT_INCLUDE_TESTS = False
    DEFAULT_INCLUDE_NON_CODE = False
    DEFAULT_INCLUDE_MAIN_CODE = False
    DEFAULT_INCLUDE_MAKEFILES = True
    DEFAULT_TEST_EXAMPLES_ONLY = True
    
    # Get current CLI args
    cli_args = sys.argv[1:]
    
    # Inject defaults if not explicitly provided
    if "--include-tests" not in cli_args and "--exclude-tests" not in cli_args:
        cli_args.append("--include-tests" if DEFAULT_INCLUDE_TESTS else "--exclude-tests")
        
    if "--include-non-code" not in cli_args and "--exclude-non-code" not in cli_args:
        cli_args.append("--include-non-code" if DEFAULT_INCLUDE_NON_CODE else "--exclude-non-code")

    if "--include-main-code" not in cli_args and "--exclude-main-code" not in cli_args:
        cli_args.append("--include-main-code" if DEFAULT_INCLUDE_MAIN_CODE else "--exclude-main-code")

    if "--include-makefiles" not in cli_args and "--exclude-makefiles" not in cli_args:
        cli_args.append("--include-makefiles" if DEFAULT_INCLUDE_MAKEFILES else "--exclude-makefiles")

    if "--test-examples-only" not in cli_args:
        if DEFAULT_TEST_EXAMPLES_ONLY:
            cli_args.append("--test-examples-only")
    
    args = parse_args(cli_args)
    
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_dir = os.path.join(root_dir, 'docs', 'dev_docs')
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_FILE)

    print(f"Generating context with:")
    print(f"  include_tests: {args.include_tests} (examples_only: {args.test_examples_only})")
    print(f"  include_non_code: {args.include_non_code}")
    print(f"  include_main_code: {args.include_main_code}")
    print(f"  include_makefiles: {args.include_makefiles}")

    with open(output_path, 'w', encoding='utf-8') as outfile:
        # AUTO-GENERATED WARNING
        outfile.write("# ⚠️ AUTO-GENERATED FILE - DO NOT EDIT MANUALLY\n\n")
        outfile.write("> **WARNING**: This file is automatically generated by `scripts/create_context.py`.\n")
        outfile.write("> Any manual edits will be overwritten the next time the script runs.\n")
        outfile.write("> To modify the content, edit the source files directly.\n\n")
        outfile.write("---\n\n")
        
        # Project header
        outfile.write("# LMUFFB Project Context\n\n")
        outfile.write("This file contains the full source code and documentation of the project.\n")
        outfile.write("It is generated automatically to provide complete context for LLM queries.\n\n")

        for dirpath, dirnames, filenames in os.walk(root_dir):
            # Modify dirnames in-place to filter directories by base name
            dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRS]

            for filename in filenames:
                if filename in EXCLUDE_FILES:
                    continue
                
                filepath = os.path.join(dirpath, filename)
                relpath = os.path.relpath(filepath, root_dir)
                relpath_normalized = relpath.replace('\\', '/')

                # Skip files in excluded relative directory paths
                if any(relpath_normalized == ex_path or relpath_normalized.startswith(ex_path + '/') for ex_path in EXCLUDE_DIR_PATHS):
                    continue

                # Categorize file
                is_code = is_code_file(filename)
                is_doc = is_doc_file(filename)
                is_make = is_make_file(filename)
                
                should_include = False
                
                # Inclusion logic
                if is_make:
                    if args.include_makefiles and not is_ignored(relpath, root_dir):
                        should_include = True
                elif is_code:
                    if is_test_file(relpath, filename):
                        if args.test_examples_only:
                            if relpath_normalized in EXAMPLE_TEST_FILES:
                                should_include = True
                        elif args.include_tests:
                            should_include = True
                    elif is_main_code(relpath_normalized):
                        if args.include_main_code:
                            should_include = True
                    else:
                        # Other code files (e.g. root scripts if not excluded)
                        should_include = True
                elif is_doc:
                    if args.include_non_code:
                        should_include = True
                
                if not should_include:
                    continue

                # Project specific hardcoded exclusions that were already here
                if 'docs/dev_docs/code_reviews' in relpath_normalized or 'docs/dev_docs/code reviews' in relpath_normalized:
                    continue

                if 'docs/dev_docs/done_features' in relpath_normalized:
                    continue

                if 'docs/dev_docs/drafts' in relpath_normalized:
                    continue

                if 'docs/dev_docs/non project guides' in relpath_normalized:
                        continue

                if 'docs/dev_docs/prompts' in relpath_normalized:
                        continue

                if 'docs/dev_docs/pending_todos' in relpath_normalized:
                    continue

                if 'docs/bug_reports' in relpath_normalized:
                    continue

                if 'tmp' in relpath_normalized:
                    continue

                if relpath_normalized == 'src/stb_image_write.h':
                    continue
                
                # Exclude .txt files in root directory except for allowed ones
                if dirpath == root_dir and filename.endswith('.txt') and not is_make:
                    allowed_root_txt = {'README.txt', 'build_commands.txt'}
                    if filename not in allowed_root_txt:
                        continue
                print(f"Adding {relpath}...")

                outfile.write(f"\n# File: {relpath}\n")
                
                # Determine language for fencing
                ext = os.path.splitext(filename)[1].lower()
                lang = ""
                if ext in ['.cpp', '.h', '.c']: lang = "cpp"
                elif ext == '.py': lang = "python"
                elif ext == '.md': lang = "markdown"
                elif ext == '.cmake' or filename == 'CMakeLists.txt': lang = "cmake"
                
                outfile.write(f"```{lang}\n")
                
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as infile:
                        content = infile.read()
                        
                        # Reformat youtube urls
                        content = re.sub(r'https?://(?:www\.)?youtube\.com/watch\?v=([\w-]+)(?:&[\w%=+-]*)?', r'youtube: \1', content)
                        content = re.sub(r'https?://youtu\.be/([\w-]+)(?:\?[\w%=+-]*)?', r'youtube: \1', content)

                        # General URL unlinking
                        def general_unlink(match):
                            url = match.group(0)
                            url = re.sub(r'^https?://', '', url, flags=re.IGNORECASE)
                            url = url.replace('.', '_')
                            return f"unlinked: {url}"

                        url_regex = r'(?:https?://|www\.)[\w\-\.\/\?\=\&\%\#\+\:]+(?<![\.\,\?\!\:\|\)\]\(`])'
                        content = re.sub(url_regex, general_unlink, content, flags=re.IGNORECASE)
                        
                        outfile.write(content)
                except Exception as e:
                    outfile.write(f"Error reading file: {e}")
                
                outfile.write("\n```\n")

    print(f"\nContext file generated: {output_path}")

if __name__ == "__main__":
    main()
