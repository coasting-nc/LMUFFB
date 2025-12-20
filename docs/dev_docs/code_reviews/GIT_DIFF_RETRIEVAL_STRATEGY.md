# Prompt to reuse when asking for a code review

Do a code review of the staged changes from git. Save the diff to a txt file for easier consultation; all saved diffs must be saved under `tmp\diffs\`. Find our any issues with the changes. Read and follow the instructions in this document on the "Git Diff Retrieval Strategy for Code Reviews": docs\dev_docs\code_reviews\GIT_DIFF_RETRIEVAL_STRATEGY.md

You can read the prompt with the instructions that were given to the coding agent at `docs\dev_docs\prompts\v_0.4.20.md` . Read this document to understand what was supposed to be implemented and verify that the staged changes fullfilled all requirements.

Save a detailed code review report in the `docs\dev_docs\code_reviews\` folder.

For build commands (also for tests) see `build_commands.txt`.

# Git Diff Retrieval Strategy for Code Reviews

**Purpose:** Best practices for retrieving git diffs on Windows PowerShell for AI-assisted code reviews  
**Platform:** Windows PowerShell  
**Date:** 2025-12-13  
**Status:** Recommended Practice

---

## Problem

When running `git diff --staged` on Windows PowerShell, the output can be truncated due to:
1. **PowerShell's default output buffering** - Limits display to terminal size
2. **Large diff sizes** - Exceeding display limits causes truncation
3. **Character encoding issues** - UTF-16LE vs UTF-8 incompatibility with text viewing tools

### Symptoms

- Diff output appears incomplete or cut off
- "unsupported mime type text/plain; charset=utf-16le" errors when trying to view files
- Missing sections of code changes
- Inability to perform comprehensive code reviews

---

## Solution: Save to File with UTF-8 Encoding

### ✅ Working Command

```powershell
git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8
```

### Why This Works

1. **Redirects to file** instead of terminal output (avoids truncation)
2. **UTF-8 encoding** ensures compatibility with text viewing tools
3. **Creates persistent file** that can be viewed with `view_file` tool
4. **No size limits** compared to terminal output
5. **Preserves complete diff** regardless of complexity or length

---

## Step-by-Step Process

### For AI Code Review Agents

```powershell
# Step 1: Save the staged diff to a file with UTF-8 encoding
git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8

# Step 2: View the file using the view_file tool
# This tool handles large files properly and won't truncate
view_file("c:\path\to\staged_changes_review.txt")

# Step 3: Perform code review analysis
# The file contains the complete, untruncated diff

# Step 4: Keep the file for documentation
# The .txt file serves as a record of what was reviewed
```

### For Unstaged Changes

```powershell
# Review all unstaged changes
git diff | Out-File -FilePath "unstaged_changes_review.txt" -Encoding UTF8

# Review specific file
git diff path/to/file.cpp | Out-File -FilePath "file_changes_review.txt" -Encoding UTF8
```

### For Commit Comparisons

```powershell
# Compare two commits
git diff commit1..commit2 | Out-File -FilePath "commit_comparison.txt" -Encoding UTF8

# Compare with remote branch
git diff origin/main..HEAD | Out-File -FilePath "branch_diff.txt" -Encoding UTF8
```

---

## Alternative Approaches

### Use --no-color Flag (Not Recommended for Large Diffs)

If you need to capture output directly:

```powershell
git diff --staged --no-color
```

**Pros:**
- Removes ANSI color codes that can cause parsing issues
- Slightly cleaner output

**Cons:**
- Still subject to terminal truncation
- Not suitable for large diffs
- No persistent record

### Use Git's Built-in Pager Control

```powershell
git --no-pager diff --staged
```

**Pros:**
- Disables pager, shows all output at once

**Cons:**
- Still truncates in PowerShell
- Overwhelming for large diffs
- No file output

---

## What NOT to Do

### ❌ Don't Use Simple Redirect

```powershell
# WRONG - Creates UTF-16LE encoding
git diff --staged > staged_changes_review.txt
```

**Problem:** This creates UTF-16LE encoding which causes "unsupported mime type" errors when viewing.

### ❌ Don't Rely on Terminal Output for Large Diffs

```powershell
# WRONG - Output will be truncated
git diff --staged
```

**Problem:** PowerShell truncates output, making comprehensive review impossible.

### ❌ Don't Use Set-Content Without Encoding

```powershell
# WRONG - May use wrong encoding
git diff --staged | Set-Content "staged_changes_review.txt"
```

**Problem:** Default encoding may not be UTF-8, causing compatibility issues.

---

## Recommended Workflow for Code Reviews

### Complete Code Review Process

```powershell
# 1. Save staged changes to file
git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8

# 2. View the complete diff
# Use view_file tool to read the entire diff without truncation

# 3. Analyze changes systematically
# - Check for logic errors
# - Verify coding standards
# - Look for potential bugs
# - Review test coverage
# - Check documentation updates

# 4. Save review findings
# Create a code review document with findings

# 5. Keep the diff file for reference
# The .txt file serves as documentation of what was reviewed
```

### Integration with AI Code Review

When requesting AI code review, include this instruction:

```
Please review the staged changes. Use this command to get the complete diff:

git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8

Then use view_file to read staged_changes_review.txt for the complete, untruncated diff.
```

---

## Advanced Usage

### Include Context Lines

```powershell
# Show more context around changes (default is 3 lines)
git diff --staged -U10 | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8
```

### Show Word-Level Diffs

```powershell
# Highlight word changes instead of line changes
git diff --staged --word-diff | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8
```

### Include Statistics

```powershell
# Show file statistics
git diff --staged --stat | Out-File -FilePath "staged_changes_stats.txt" -Encoding UTF8

# Show detailed statistics
git diff --staged --numstat | Out-File -FilePath "staged_changes_numstat.txt" -Encoding UTF8
```

### Filter by File Type

```powershell
# Only C++ files
git diff --staged -- "*.cpp" "*.h" | Out-File -FilePath "cpp_changes.txt" -Encoding UTF8

# Exclude certain files
git diff --staged -- . ":(exclude)*.md" | Out-File -FilePath "code_only_changes.txt" -Encoding UTF8
```

---

## Troubleshooting

### Issue: "Out-File: Cannot bind parameter 'Encoding'"

**Cause:** Older PowerShell version  
**Solution:** Use `-Encoding utf8` (lowercase) or upgrade PowerShell

```powershell
git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding utf8
```

### Issue: File is empty or incomplete

**Cause:** No staged changes or git error  
**Solution:** Check git status first

```powershell
# Verify there are staged changes
git status

# Check if diff command works
git diff --staged --name-only
```

### Issue: Special characters appear garbled

**Cause:** Encoding mismatch  
**Solution:** Ensure UTF-8 encoding is used

```powershell
# Explicitly set UTF-8
git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8
```

---

## Best Practices Summary

### ✅ DO

- **Always save to file** for code reviews (don't rely on terminal output)
- **Use UTF-8 encoding** explicitly with `Out-File -Encoding UTF8`
- **Use `view_file` tool** to read the saved diff (handles large files properly)
- **Keep the diff file** as part of the code review documentation
- **Use descriptive filenames** (e.g., `v0.4.10_staged_changes.txt`)
- **Include timestamp** in filename for historical tracking

### ❌ DON'T

- Don't use simple redirect (`>`) - creates wrong encoding
- Don't rely on terminal output - will truncate
- Don't use `Set-Content` without encoding parameter
- Don't delete diff files immediately - keep for reference
- Don't assume small diffs won't truncate - always use file output

---

## Quick Reference Card

```powershell
# ============================================
# Git Diff Retrieval - Quick Reference
# ============================================

# Staged changes (most common for code review)
git diff --staged | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8

# Unstaged changes
git diff | Out-File -FilePath "unstaged_changes_review.txt" -Encoding UTF8

# Specific file
git diff path/to/file | Out-File -FilePath "file_changes.txt" -Encoding UTF8

# With more context
git diff --staged -U10 | Out-File -FilePath "staged_changes_review.txt" -Encoding UTF8

# Statistics only
git diff --staged --stat | Out-File -FilePath "changes_stats.txt" -Encoding UTF8

# View the file (in AI agent)
view_file("c:\path\to\staged_changes_review.txt")
```

---

## Integration with Development Workflow

### Pre-Commit Review

```powershell
# Before committing, save diff for review
git diff --staged | Out-File -FilePath "pre_commit_review_$(Get-Date -Format 'yyyyMMdd_HHmmss').txt" -Encoding UTF8

# Review the changes
# ... perform code review ...

# Commit if review passes
git commit -m "Your commit message"
```

### Pull Request Preparation

```powershell
# Compare your branch with main
git diff main..HEAD | Out-File -FilePath "pr_changes_$(Get-Date -Format 'yyyyMMdd').txt" -Encoding UTF8

# Review before creating PR
# ... perform code review ...

# Create PR if review passes
```

---

## Key Takeaways

1. **Always save to file** for code reviews - terminal output is unreliable
2. **Use UTF-8 encoding** explicitly to avoid compatibility issues
3. **Use `Out-File -Encoding UTF8`** - not simple redirect (`>`)
4. **Keep diff files** as documentation of what was reviewed
5. **This approach works** regardless of diff size or complexity

This strategy ensures you get the **complete, untruncated diff** every time, enabling thorough and accurate code reviews.

---

**Document Version:** 1.0  
**Last Updated:** 2025-12-13  
**Platform:** Windows PowerShell 5.1+  
**Status:** Recommended Practice
