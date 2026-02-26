import xml.etree.ElementTree as ET
import sys
import os

def parse_cobertura(xml_path, metric="line"):
    if not os.path.exists(xml_path):
        print(f"Error: {xml_path} not found.")
        return

    tree = ET.parse(xml_path)
    root = tree.getroot()

    summary = []

    for clazz in root.findall(".//class"):
        filename = clazz.get("filename")
        
        # Exclude vendor, dependency, and test code (Python specifically)
        if "venv" in filename or "tests/" in filename or "__init__.py" in filename:
            continue

        if metric == "line":
            rate = float(clazz.get("line-rate"))
        elif metric == "branch":
            rate = float(clazz.get("branch-rate"))
        elif metric == "function":
            # Cobertura doesn't always have a direct 'function-rate', 
            # but we can check methods hits.
            methods = clazz.findall(".//method")
            if not methods:
                continue
            total_methods = len(methods)
            covered_methods = sum(1 for m in methods if any(l.get("hits") != "0" for l in m.findall(".//line")))
            rate = covered_methods / total_methods if total_methods > 0 else 1.0

        missing = []
        if metric == "line":
            items = clazz.find("lines")
            if items is not None:
                for item in items.findall("line"):
                    if item.get("hits") == "0":
                        missing.append(int(item.get("number")))
        elif metric == "branch":
            items = clazz.find("lines")
            if items is not None:
                for item in items.findall("line"):
                    if item.get("branch") == "true":
                        condition_coverage = item.get("condition-coverage")
                        if condition_coverage and "100%" not in condition_coverage:
                            missing.append(int(item.get("number")))
        elif metric == "function":
            methods = clazz.findall(".//method")
            for m in methods:
                hit = any(l.get("hits") != "0" for l in m.findall(".//line"))
                if not hit:
                    missing.append(m.get("name"))

        if not missing:
            if rate == 1.0:
                summary.append(f"File: {filename}\n  {metric.capitalize()} Coverage: 100%\n")
            else:
                summary.append(f"File: {filename}\n  {metric.capitalize()} Coverage: {rate*100:.1f}% (No specific missing {metric}s found)\n")
            continue

        label = "Lines" if metric == "line" else "Branches" if metric == "branch" else "Functions"
        
        if metric in ["line", "branch"]:
            # Group into ranges
            ranges = []
            if missing:
                missing = sorted(list(set(missing)))
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
            missing_str = ', '.join(ranges)
        else:
            missing_str = ', '.join(missing)

        summary.append(f"File: {filename}\n  {metric.capitalize()} Coverage: {rate*100:.1f}%\n  Missing {label}: {missing_str}\n")

    return "\n".join(summary)

if __name__ == "__main__":
    path = "python_cobertura.xml"
    if len(sys.argv) > 1:
        path = sys.argv[1]
    
    line_report = parse_cobertura(path, "line")
    branch_report = parse_cobertura(path, "branch")
    function_report = parse_cobertura(path, "function")

    print(f"=== Python Line Coverage Summary ===")
    with open("python_coverage_summary.txt", "w") as f:
        if line_report:
            print(line_report)
            f.write(line_report)
        else:
            f.write("No line coverage data available.")
    
    print(f"\n=== Python Branch Coverage Summary ===")
    with open("python_coverage_branches_summary.txt", "w") as f:
        if branch_report:
            print(branch_report)
            f.write(branch_report)
        else:
            f.write("No branch coverage data available.")

    print(f"\n=== Python Function Coverage Summary ===")
    with open("python_coverage_functions_summary.txt", "w") as f:
        if function_report:
            print(function_report)
            f.write(function_report)
        else:
            f.write("No function coverage data available or methods not exported in XML.")
