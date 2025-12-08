import os

OUTPUT_FILE = "FULL_PROJECT_CONTEXT.md"

# Extensions to include
EXTENSIONS = {
    '.cpp', '.h', '.c', '.hpp', 
    '.md', '.txt', 
    '.iss', '.cmake', 'CMakeLists.txt',
    '.py'
}

# Directories to exclude
EXCLUDE_DIRS = {
    'build', 'build_tests', '.git', 'python_prototype', 'vendor', '__pycache__', '.vscode', '.specstory'
}

# Files to exclude
EXCLUDE_FILES = {
    OUTPUT_FILE, 'LICENSE', 'compile_commands.json'
}

def is_text_file(filename):
    return any(filename.endswith(ext) or filename == ext for ext in EXTENSIONS)

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_dir = os.path.join(root_dir, 'docs', 'dev_docs')
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, OUTPUT_FILE)

    with open(output_path, 'w', encoding='utf-8') as outfile:
        outfile.write("# LMUFFB Project Context\n\n")
        outfile.write("This file contains the full source code and documentation of the project.\n\n")

        for dirpath, dirnames, filenames in os.walk(root_dir):
            # Modify dirnames in-place to filter directories
            dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRS]

            for filename in filenames:
                if filename in EXCLUDE_FILES:
                    continue
                
                if not is_text_file(filename):
                    continue

                filepath = os.path.join(dirpath, filename)
                relpath = os.path.relpath(filepath, root_dir)

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
                        outfile.write(infile.read())
                except Exception as e:
                    outfile.write(f"Error reading file: {e}")
                
                outfile.write("\n```\n")

    print(f"\nContext file generated: {output_path}")

if __name__ == "__main__":
    main()
