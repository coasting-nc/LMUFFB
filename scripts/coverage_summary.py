import xml.etree.ElementTree as ET
import sys
import os

def parse_cobertura(xml_path):
    if not os.path.exists(xml_path):
        print(f"Error: {xml_path} not found.")
        return

    tree = ET.parse(xml_path)
    root = tree.getroot()

    summary = []

    for clazz in root.findall(".//class"):
        filename = clazz.get("filename")
        line_rate = float(clazz.get("line-rate"))
        
        missing_lines = []
        lines = clazz.find("lines")
        if lines is not None:
            for line in lines.findall("line"):
                if line.get("hits") == "0":
                    missing_lines.append(int(line.get("number")))

        if not missing_lines:
            if line_rate == 1.0:
                summary.append(f"File: {filename}\n  Coverage: 100%\n")
            else:
                # This might happen if there are no executable lines tracked
                summary.append(f"File: {filename}\n  Coverage: {line_rate*100:.1f}% (No specific missing lines found)\n")
            continue

        # Group into ranges
        ranges = []
        if missing_lines:
            start = missing_lines[0]
            prev = start
            for i in range(1, len(missing_lines)):
                if missing_lines[i] == prev + 1:
                    prev = missing_lines[i]
                else:
                    if start == prev:
                        ranges.append(f"{start}")
                    else:
                        ranges.append(f"{start}-{prev}")
                    start = missing_lines[i]
                    prev = start
            if start == prev:
                ranges.append(f"{start}")
            else:
                ranges.append(f"{start}-{prev}")

        summary.append(f"File: {filename}\n  Coverage: {line_rate*100:.1f}%\n  Missing Lines: {', '.join(ranges)}\n")

    return "\n".join(summary)

if __name__ == "__main__":
    path = "cobertura.xml"
    if len(sys.argv) > 1:
        path = sys.argv[1]
    
    report = parse_cobertura(path)
    if report:
        print("=== C++ Code Coverage AI-Friendly Summary ===")
        print(report)
        
        # Also save to a file
        with open("coverage_summary.txt", "w") as f:
            f.write(report)
        print(f"\nSummary saved to coverage_summary.txt")
