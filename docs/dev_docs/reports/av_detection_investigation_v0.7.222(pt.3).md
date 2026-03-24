


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


# Adding VirusTotal Scan

Separating slow, comprehensive malware scans into a different workflow is a standard and highly recommended CI/CD practice. It keeps your developer feedback loop fast while still providing deep security insights asynchronously.

Here is how you can structure this:

### 1. The Fast Scan (Keep in Main Workflow)
The **Windows Defender (`MpCmdRun.exe`)** step we discussed previously is the perfect "Fast Scan." It takes only a few seconds, runs locally on the GitHub runner, and catches structural/static heuristic flags immediately. You should keep this in your main `windows-build-and-test.yml` workflow.

### 2. The Slow Scan (Separate VirusTotal Workflow)
You can fully automate the VirusTotal scan using a separate GitHub Actions workflow. By using the `workflow_run` trigger, this new workflow will automatically start **only after** your main "Build and Test" workflow finishes successfully. 

Because it is a separate workflow, **it will not slow down your main build**, and you can configure it to act as a warning (it won't block your pull requests or releases).

#### Step 1: Get a Free VirusTotal API Key
1. Create a free account at [VirusTotal](https://www.virustotal.com/).
2. Go to your profile -> **API Key**.
3. Copy the key.
4. Go to your GitHub Repository -> **Settings** -> **Secrets and variables** -> **Actions** -> **New repository secret**.
5. Name it `VT_API_KEY` and paste your key.

#### Step 2: Create the VirusTotal Workflow
Create a new file in your repository at `.github/workflows/virustotal-scan.yml` and paste the following code:

```yaml
name: VirusTotal Scan

# Trigger this workflow automatically when the main build completes
on:
  workflow_run:
    workflows: ["Build and Test"]
    types:
      - completed

permissions:
  contents: read

jobs:
  scan:
    name: VirusTotal Comprehensive Scan
    runs-on: ubuntu-latest
    # Only run if the main build actually succeeded
    if: ${{ github.event.workflow_run.conclusion == 'success' }}
    
    steps:
      - name: Download Nightly Artifact
        uses: actions/download-artifact@v4
        with:
          # Download the artifact from the workflow run that triggered this
          run-id: ${{ github.event.workflow_run.id }}
          github-token: ${{ secrets.GITHUB_TOKEN }}
          # Use a pattern to catch the dynamic version name (e.g., lmuffb-0.7.223-nightly)
          pattern: lmuffb-*-nightly
          path: temp_download
          merge-multiple: true

      - name: Extract Executable
        run: |
          # Find the zip file
          ZIP_FILE=$(ls temp_download/*.zip | head -n 1)
          echo "Found artifact: $ZIP_FILE"
          
          # Unzip just the executable for scanning
          unzip -j "$ZIP_FILE" "LMUFFB.exe" -d scan_dir/
          
      - name: VirusTotal Scan
        uses: crazy-max/ghaction-virustotal@v4
        id: vt
        with:
          vt_api_key: ${{ secrets.VT_API_KEY }}
          # Path to the extracted executable
          files: scan_dir/LMUFFB.exe
          # Request a full analysis (this handles the 20+ minute wait automatically)
          request_rate: 4
        # Treat this as a warning: if VT flags it, the step fails, but the workflow continues
        continue-on-error: true

      - name: Print VirusTotal Results
        run: |
          echo "### VirusTotal Scan Results 🦠" >> $GITHUB_STEP_SUMMARY
          echo "**File:** LMUFFB.exe" >> $GITHUB_STEP_SUMMARY
          echo "**Positives:** ${{ steps.vt.outputs.positives }} / ${{ steps.vt.outputs.total }}" >> $GITHUB_STEP_SUMMARY
          echo "**Report Link:** [View Full Report on VirusTotal](${{ steps.vt.outputs.permalink }})" >> $GITHUB_STEP_SUMMARY
          
          if[ "${{ steps.vt.outputs.positives }}" -gt 0 ]; then
            echo "⚠️ **Warning:** Some engines flagged this file. Check the report link for details." >> $GITHUB_STEP_SUMMARY
          else
            echo "✅ **Clean:** No engines flagged this file." >> $GITHUB_STEP_SUMMARY
          fi
```

### How this works:
1. **Asynchronous Execution:** The `on: workflow_run` trigger means this YAML file sits completely idle until your main `windows-build-and-test.yml` finishes. Your developers get their build success/failure feedback immediately.
2. **Artifact Retrieval:** It uses the GitHub API to reach back into the finished build, grab the `.zip` file you uploaded, and extract `LMUFFB.exe`.
3. **Automated Polling:** The `crazy-max/ghaction-virustotal` action uploads the file to VirusTotal and automatically handles the polling (waiting for the 70+ engines to finish scanning). 
4. **Non-Blocking Warnings:** Because of `continue-on-error: true`, if 2 out of 70 engines flag the file (a common false-positive scenario for indie C++ apps), the step will show a red `X` in the Actions tab, but it won't break your repository status.
5. **Summary Report:** It prints a nice markdown summary directly into the GitHub Actions UI with a clickable link to the full VirusTotal web report, saving you from having to upload it manually ever again.

### Important Note on VirusTotal Free API Limits
The free VirusTotal API allows **4 requests per minute** and **500 requests per day**. 
If you push commits very rapidly (e.g., 5 commits in 2 minutes), the VT action might hit the rate limit and fail. Because we set `continue-on-error: true`, this won't break anything, but it's something to be aware of if you see the VT workflow occasionally failing during heavy development sessions.