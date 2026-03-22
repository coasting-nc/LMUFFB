import os
import sys

def main():
    presets_dir = "assets/builtin_presets"
    output_file = "src/core/GeneratedBuiltinPresets.h"

    if not os.path.exists(presets_dir):
        print(f"Error: {presets_dir} not found")
        sys.exit(1)

    presets = {}
    for filename in os.listdir(presets_dir):
        if filename.endswith(".toml"):
            name = filename[:-5]
            path = os.path.join(presets_dir, filename)
            with open(path, "r", encoding="utf-8") as f:
                content = f.read()
                presets[name] = content

    with open(output_file, "w", encoding="utf-8") as f:
        f.write("// AUTO-GENERATED FILE - DO NOT EDIT\n")
        f.write("#pragma once\n")
        f.write("#include <string_view>\n")
        f.write("#include <map>\n\n")
        f.write("inline const std::map<std::string_view, std::string_view> BUILTIN_PRESETS = {\n")

        items = list(presets.items())
        for i, (name, content) in enumerate(items):
            f.write(f'    {{"{name}", R"PRESET(')
            f.write(content)
            f.write(')PRESET"}')
            if i < len(items) - 1:
                f.write(",")
            f.write("\n")

        f.write("};\n")

if __name__ == "__main__":
    main()
