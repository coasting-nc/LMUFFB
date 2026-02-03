# Unicode Encoding Issues in Code Files

## Overview

This document describes encoding issues that can occur when working with source code files on Windows, particularly in the context of AI agent tools that process file content.

---

## Problem Description

### Symptoms

When attempting to read certain files using agent tools (like `view_file`), you may encounter errors such as:

```
Error: unsupported mime type text/plain; charset=utf-16le
```

This prevents the agent from reading or editing the affected files.

### Root Cause

The issue occurs when source files are saved with **UTF-16LE (Little Endian)** encoding instead of the more commonly expected **UTF-8** encoding. This can happen due to:

1. **PowerShell Output Redirection**: Some PowerShell commands output UTF-16LE by default
2. **Editor Settings**: Some editors may save files in non-UTF-8 encodings
3. **Copy Operations**: Copying between systems with different default encodings
4. **Git Operations**: Certain Git merge/conflict resolution tools may alter encoding
5. **File Generation Tools**: Some code generation tools output non-UTF-8 files

### Affected File Types

Any text file can be affected, but commonly observed cases include:

- `.cpp` source files
- `.h` header files  
- `.txt` text files
- `.md` markdown files
- Configuration files

---

## Detection

### Check File Encoding (PowerShell)

```powershell
# Check if file has BOM (Byte Order Mark)
$bytes = [System.IO.File]::ReadAllBytes("path\to\file.cpp")
if ($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
    Write-Host "UTF-16LE (with BOM)"
} elseif ($bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
    Write-Host "UTF-8 (with BOM)"
} else {
    Write-Host "No BOM detected (likely UTF-8 or ASCII)"
}
```

### Check File Encoding (Git Bash / WSL)

```bash
file path/to/file.cpp
# Output examples:
# "UTF-8 Unicode text"
# "UTF-16 Unicode text, with very long lines, little-endian"
```

---

## Solutions

### Solution 1: Convert to UTF-8 using PowerShell

```powershell
# Read the content and save as UTF-8
Get-Content "path\to\file.cpp" | Out-File -FilePath "path\to\file_utf8.cpp" -Encoding utf8

# Or overwrite in place:
$content = Get-Content "path\to\file.cpp" -Raw
[System.IO.File]::WriteAllText("path\to\file.cpp", $content, [System.Text.Encoding]::UTF8)
```

### Solution 2: Convert to UTF-8 using PowerShell (with BOM)

```powershell
# UTF-8 with BOM (recommended for Windows compatibility)
$content = Get-Content "path\to\file.cpp" -Raw
$utf8BOM = New-Object System.Text.UTF8Encoding $true
[System.IO.File]::WriteAllText("path\to\file.cpp", $content, $utf8BOM)
```

### Solution 3: Convert using Notepad++

1. Open the file in Notepad++
2. Go to **Encoding** menu
3. Select **Convert to UTF-8** (or **UTF-8-BOM**)
4. Save the file

### Solution 4: Convert using Visual Studio Code

1. Open the file in VS Code
2. Click on the encoding indicator in the bottom status bar (e.g., "UTF-16 LE")
3. Select **"Save with Encoding"**
4. Choose **"UTF-8"** or **"UTF-8 with BOM"**

### Solution 5: Batch Convert Multiple Files

```powershell
# Convert all .cpp files in a directory to UTF-8
Get-ChildItem -Path ".\tests" -Filter "*.cpp" | ForEach-Object {
    $content = Get-Content $_.FullName -Raw
    [System.IO.File]::WriteAllText($_.FullName, $content, [System.Text.Encoding]::UTF8)
    Write-Host "Converted: $($_.Name)"
}
```

---

## Prevention

### Best Practices

1. **Configure Your Editor**
   - Set default encoding to UTF-8 for all new files
   - VS Code: Add to `settings.json`:
     ```json
     "files.encoding": "utf8",
     "files.autoGuessEncoding": false
     ```

2. **PowerShell Output**
   - Always specify encoding when writing files:
     ```powershell
     # Good: explicit encoding
     "content" | Out-File -Encoding utf8 file.txt
     
     # Bad: default encoding (may be UTF-16)
     "content" > file.txt
     ```

3. **Git Configuration**
   - Consider adding a `.gitattributes` file:
     ```
     * text=auto eol=lf
     *.cpp text encoding=utf-8
     *.h text encoding=utf-8
     ```

4. **EditorConfig**
   - Add or update `.editorconfig`:
     ```ini
     [*]
     charset = utf-8
     
     [*.{cpp,h,hpp}]
     charset = utf-8
     ```

5. **Pre-Commit Hooks**
   - Add a pre-commit hook to detect non-UTF-8 files before committing

---

## Workaround for Agents

When encountering encoding issues during an agent session:

1. **Create a UTF-8 copy**:
   ```powershell
   Get-Content "file.cpp" | Out-File -FilePath "file_utf8.cpp" -Encoding utf8
   ```

2. **Work with the UTF-8 copy**

3. **When finished, copy back** (if needed):
   ```powershell
   Copy-Item "file_utf8.cpp" -Destination "file.cpp" -Force
   ```

4. **Clean up temporary files**:
   ```powershell
   Remove-Item "file_utf8.cpp"
   ```

---

## Related Resources

- [UTF-8 Everywhere Manifesto](http://utf8everywhere.org/)
- [Microsoft: Character encoding in .NET](https://docs.microsoft.com/en-us/dotnet/standard/base-types/character-encoding)
- [Git - gitattributes Documentation](https://git-scm.com/docs/gitattributes)

---

*Document created: 2026-02-01*  
*Related implementation note: `docs/dev_docs/plans/plan_slope_detection.md` - Implementation Notes section*
