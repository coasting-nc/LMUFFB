


Yes, you can absolutely add a Windows Defender scan to your GitHub Actions workflows! 

GitHub's `windows-latest` runners (which currently use Windows Server 2022) have Microsoft Defender Antivirus installed and running by default. You can trigger a manual scan on your compiled `.exe` using Defender's command-line utility, `MpCmdRun.exe`.

Here is how to implement it, along with an important caveat about how ML heuristics work in CI environments.

### The Windows Defender Scan Step

You can add this step right after your `Configure and Build (Release)` step (and after your code signing step, if you implemented the self-signed certificate). 

Add this block to both your `.github\workflows\windows-build-and-test.yml` and `.github\workflows\manual-release.yml`:

```yaml
    - name: Windows Defender Scan
      shell: pwsh
      run: |
        $exePath = "build\Release\LMUFFB.exe"
        
        # 1. Check if the file even exists. If Defender caught it during compilation, 
        # it might have already silently quarantined/deleted it.
        if (-not (Test-Path $exePath)) {
            Write-Error "Executable not found! It may have been quarantined by Defender during the build."
            exit 1
        }
        
        $absPath = (Resolve-Path $exePath).Path
        Write-Host "Scanning $absPath with Windows Defender..."
        
        # 2. Run the Defender command-line utility (ScanType 3 is a custom file/directory scan)
        & "C:\Program Files\Windows Defender\MpCmdRun.exe" -Scan -ScanType 3 -File "$absPath"
        
        # 3. Check the exit code. 
        # 0 = Clean, 2 = Threat Detected
        if ($LASTEXITCODE -eq 2) {
            Write-Error "Windows Defender detected a threat! Workflow failed."
            exit 1
        } elseif ($LASTEXITCODE -eq 0) {
            Write-Host "Windows Defender scan passed. No threats detected."
        } else {
            Write-Warning "Windows Defender scan returned unexpected exit code: $LASTEXITCODE"
        }
```

### Where to place it in your workflows

**In `windows-build-and-test.yml`:**
Place it right after the build step, before you run the tests (because if it's malware, you don't want to run it, and if Defender quarantines it, the tests will fail anyway).

```yaml
    - name: Configure and Build (Release)
      shell: pwsh
      run: |
        cmake -S . -B build
        cmake --build build --config Release --clean-first

    - name: Windows Defender Scan
      shell: pwsh
      run: |
        # ... (paste the script here) ...

    - name: Run Tests
      shell: pwsh
      run: |
        .\build\tests\Release\run_combined_tests.exe
```

**In `manual-release.yml`:**
Place it right after the build step (or after the signing step, if you added one).

```yaml
      - name: Configure and Build (Release)
        shell: pwsh
        run: |
          cmake -S . -B build
          cmake --build build --config Release --clean-first

      - name: Windows Defender Scan
        shell: pwsh
        run: |
          # ... (paste the script here) ...

      - name: Run Tests
        shell: pwsh
        run: |
          .\build\tests\Release\run_combined_tests.exe
```

### ⚠️ Important Caveat: CI Defender vs. Consumer Defender

While adding this step is a fantastic baseline check, you must be aware of a limitation regarding **Machine Learning (ML) heuristics** (like the `!ml` tag in your `Program:Win32/Wacapew.A!ml` detection).

1. **Cloud-Delivered Protection:** The most aggressive ML heuristics rely on Microsoft's Cloud-Delivered Protection. GitHub Actions runners are ephemeral virtual machines. While they have Defender enabled, they do not always have the aggressive, real-time cloud ML features fully spun up or synced in the same way a consumer's Windows 11 desktop does.
2. **Behavioral vs. Static:** `MpCmdRun.exe` performs a **static scan** of the file on disk. It looks at the imports, the entropy, and the structure (which your `.rc` and `<psapi.h>` fixes will successfully pass). However, it cannot fully simulate the **behavioral scan** that happens when a user actually double-clicks the `.exe` and the OS watches it spawn `cmd.exe` (which your `ShellExecuteW` fix resolves).

**Conclusion:** 
Adding this step is highly recommended. It will immediately catch standard signature detections and structural heuristic flags, failing your PRs before they merge. Just keep in mind that a "Pass" in GitHub Actions doesn't guarantee a 100% pass on a user's machine, which is why submitting false positives to Microsoft and eventually getting a Code Signing Certificate remain the ultimate solutions.