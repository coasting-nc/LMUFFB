---
name: code_coverage_optimization
description: Strategies for increasing code coverage and generating coverage reports
---

# Code Coverage Optimization

Use this skill when tasked with improving code coverage (Line, Branch, or Function).

## 🎯 Coverage Prioritization
Prioritize "low hanging fruits" and high-impact areas:
1.  **Low Hanging Fruit**: Meaningful and easy-to-write tests that increase coverage immediately.
2.  **Absolute Uncovered Count**: Target code files with the highest absolute number of lines or branches that are currently NOT covered.
3.  **Lowest Percentage Coverage**: Target files with the lowest line or branch coverage percentages.

*Note: The overall goal is to increase general code coverage to a good level, starting from the easiest and most impactful tests to write.*

## 🛠️ The Process

### 1. Baseline Verification
- **Regenerate Summaries**: Before changing code, run `scripts\coverage_summary.py` to get a fresh baseline.
- **Consult Reports**: Check `docs\dev_docs\reports\coverage\` for uncovered lines/branches/functions.

### 2. Implementation
- **Meaningful Tests**: Focus on testing actual functionality; do not add "empty" tests just to inflate numbers.
- **Maintain Main Code**: Avoid changing the main production code unless necessary for testability (e.g., unreachable branches); instead, use TODO notes for later.
- **Skip if Impractical**: If a test for a particular coverage aspect is too complex or time-consuming, skip it and focus on easier, high-impact tests.

### 3. Reporting
- **Coverage Report**: Include a detailed `.md` report discussing why certain coverages are challenging for each file/type.
- **Discrepancy Reporting**: Document and explain any differences between your claims and code review findings.
- **Regenerate & Include**: Update and include the recalculated txt summaries in your final PR/patch.

## Commands
- **Regenerate Reports**: `python scripts/coverage_summary.py`
- **View Summaries**:
  - `docs\dev_docs\reports\coverage\coverage_summary.txt` (Lines)
  - `docs\dev_docs\reports\coverage\coverage_branches_summary.txt` (Branches)
  - `docs\dev_docs\reports\coverage\coverage_functions_summary.txt` (Functions)
