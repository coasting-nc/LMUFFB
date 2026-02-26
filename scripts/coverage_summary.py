import xml.etree.ElementTree as ET
import sys
import os
import json

def parse_cobertura(xml_path, metric="line"):
    if not os.path.exists(xml_path):
        print(f"Error: {xml_path} not found.")
        return

    tree = ET.parse(xml_path)
    root = tree.getroot()

    summary = []

    for clazz in root.findall(".//class"):
        filename = clazz.get("filename")
        
        # Exclude vendor, dependency, and test code
        if "vendor" in filename or "_deps" in filename or "tests/" in filename:
            continue

        if metric == "line":
            rate = float(clazz.get("line-rate"))
        else:
            rate = float(clazz.get("branch-rate"))

        missing = []
        items = clazz.find("lines")
        if items is not None:
            for item in items.findall("line"):
                if metric == "line":
                    if item.get("hits") == "0":
                        missing.append(int(item.get("number")))
                else:
                    # branch-rate check for each line
                    if item.get("branch") == "true":
                        condition_coverage = item.get("condition-coverage")
                        # Format: "50% (1/2)"
                        if condition_coverage and "100%" not in condition_coverage:
                            missing.append(int(item.get("number")))

        if not missing:
            if rate == 1.0:
                summary.append(f"File: {filename}\n  {metric.capitalize()} Coverage: 100%\n")
            else:
                summary.append(f"File: {filename}\n  {metric.capitalize()} Coverage: {rate*100:.1f}% (No specific missing {metric}s found)\n")
            continue

        # Group into ranges
        ranges = []
        if missing:
            start = missing[0]
            prev = start
            for i in range(1, len(missing)):
                if missing[i] == prev + 1:
                    prev = missing[i]
                else:
                    if start == prev:
                        ranges.append(f"{start}")
                    else:
                        ranges.append(f"{start}-{prev}")
                    start = missing[i]
                    prev = start
            if start == prev:
                ranges.append(f"{start}")
            else:
                ranges.append(f"{start}-{prev}")

        label = "Lines" if metric == "line" else "Branches"
        summary.append(f"File: {filename}\n  {metric.capitalize()} Coverage: {rate*100:.1f}%\n  Missing {label}: {', '.join(ranges)}\n")

    return "\n".join(summary)

def parse_json_summary(json_path):
    if not os.path.exists(json_path):
        print(f"Warning: {json_path} not found. Function coverage will be skipped.")
        return None

    with open(json_path, "r") as f:
        data = json.load(f)

    summary = []
    # Files are in data["files"]
    for file_data in data.get("files", []):
        filename = file_data.get("filename")

        # Exclude vendor, dependency, and test code
        if "vendor" in filename or "_deps" in filename or "tests/" in filename:
            continue

        rate = file_data.get("function_percent", 0.0)
        total = file_data.get("function_total", 0)
        covered = file_data.get("function_covered", 0)

        summary.append(f"File: {filename}\n  Function Coverage: {rate:.1f}% ({covered}/{total})\n")

    return "\n".join(summary)

if __name__ == "__main__":
    cobertura_path = "cobertura.xml"
    json_path = "summary.json"

    if len(sys.argv) > 1:
        cobertura_path = sys.argv[1]
    if len(sys.argv) > 2:
        json_path = sys.argv[2]
    
    line_report = parse_cobertura(cobertura_path, "line")
    branch_report = parse_cobertura(cobertura_path, "branch")
    function_report = parse_json_summary(json_path)

    if line_report:
        print("=== C++ Line Coverage Summary (Excluding Vendor/Tests) ===")
        print(line_report)
        with open("coverage_summary.txt", "w") as f:
            f.write(line_report)
        print(f"\nSummary saved to coverage_summary.txt")

    if branch_report:
        print("\n=== C++ Branch Coverage Summary (Excluding Vendor/Tests) ===")
        print(branch_report)
        with open("coverage_branches_summary.txt", "w") as f:
            f.write(branch_report)
        print(f"Summary saved to coverage_branches_summary.txt")

    if function_report:
        print("\n=== C++ Function Coverage Summary (Excluding Vendor/Tests) ===")
        print(function_report)
        with open("coverage_functions_summary.txt", "w") as f:
            f.write(function_report)
        print(f"Summary saved to coverage_functions_summary.txt")
