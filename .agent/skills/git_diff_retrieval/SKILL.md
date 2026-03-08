---
name: git_diff_retrieval
description: Best practices for retrieving git diffs on Windows PowerShell for AI-assisted code reviews
---

# Git Diff Retrieval

Use this skill to successfully retrieve complete, untruncated git diffs on Windows PowerShell, avoiding encoding issues.

## Problem
On Windows PowerShell:
1. Output can be truncated due to buffering.
2. Large diffs exceed display limits.
3. Encoding issues (UTF-16LE) cause "unsupported mime type" errors.

## Recommended Workflow

### 1. Save Staged Changes to File
Always save the diff to a file with UTF-8 encoding.

```powershell
git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8
```

### 2. View the File
Use the `view_file` tool to read the generated file. This handles large files correctly.

### 3. Clean Up
Delete the temporary diff file after the review is complete (or keep it if requested for documentation).

## Troubleshooting

### Character Encoding Errors
If you see "unsupported mime type text/plain; charset=utf-16le", it means the file was created with default PowerShell redirection (`>`). Use `Out-File -Encoding UTF8` instead.

### Truncated Output
If the diff seems incomplete, ensure you are redirecting to a file and not just reading the terminal output.

## Alternative Commands

- **Unstaged changes:**
  ```powershell
  git diff | Out-File -FilePath "unstaged_changes_review.txt" -Encoding UTF8
  ```
- **Specific file:**
  ```powershell
  git diff path/to/file.cpp | Out-File -FilePath "file_changes_review.txt" -Encoding UTF8
  ```
- **Compare commits:**
  ```powershell
  git diff commit1..commit2 | Out-File -FilePath "commit_comparison.txt" -Encoding UTF8
  ```
