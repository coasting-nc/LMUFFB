# Prompt: Tackle `bugprone-narrowing-conversions`

I'm working on resolving Clang-Tidy warnings for the LMUFFB project.

The key files are:
- **Suppression backlog report**: `docs/dev_docs/reports/clang_tidy_suppressions_backlog.md`
- **CI workflow**: `.github/workflows/windows-build-and-test.yml`
- **Local build commands**: `build_commands.txt`

The next item you have to work on is **`readability-magic-numbers`**.


on the **Top 10 High Priority** list is **`bugprone-narrowing-conversions`** (under the Medium Priority list in the report).

Please follow the workflow below to resolve the warnings. The workflow described below has examples based on based `bugprone-narrowing-conversions`, but you have to replace the examples with the appropriate examples for `readability-magic-numbers`.

---

## Step 1 — Re-enable the warning

Remove `-bugprone-narrowing-conversions` from the suppressed list in:
- `build_commands.txt` (search for the string and remove it from the `-checks=` argument)
- `.github/workflows/windows-build-and-test.yml` (same — find and remove the suppression from the clang-tidy check step)

---

## Step 2 — Run targeted static analysis

Use a fast targeted scan rather than a full build. Use the `build_ninja` compile database:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation
$env:PATH += ";C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin"
Get-ChildItem -Path src\*.cpp, tests\*.cpp -Recurse | ForEach-Object { clang-tidy -p build_ninja $_.FullName --extra-arg=/EHsc -checks="-*,bugprone-narrowing-conversions" } | Out-File narrowing_warns.txt -Encoding utf8
```

Then inspect the results:

```powershell
Select-String -Path narrowing_warns.txt -Pattern "narrowing-conversions"
```

---

## Step 3 — Fix all warnings

The general fix for narrowing conversions is to use explicit casts to make intent clear:
- `double` → `float`: use `static_cast<float>(...)`
- `float` → `int`: use `static_cast<int>(...)`
- `size_t` → `int`: use `static_cast<int>(...)`

Prioritize physics/math code (e.g. `FFBEngine.cpp`, `GuiLayer_Common.cpp`) where precision loss is most risky.

---

## Step 4 — Verify

Re-run the targeted scan and confirm zero warnings remain for this check.

---

## Step 5 — Update the backlog report

In `docs/dev_docs/reports/clang_tidy_suppressions_backlog.md`:
- Strike through item #8: `~~bugprone-narrowing-conversions~~`
- Mark it ✅ **RESOLVED in X.Y.Z**
- Update the action note with what was actually done

---

## Step 6 — Increment version

Bump the rightmost version number by +1 (e.g. `0.7.103` → `0.7.104`) in:
- `VERSION`
- `src/Version.h`

---

## Step 7 — Update changelogs

Add a new entry to both:
- `CHANGELOG_DEV.md` — technical description of each fix
- `USER_CHANGELOG.md` — user-facing release note (BBCode forum format)

---

## Build & Test commands (for reference)

```powershell
# Full build
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release

# Run all tests
.\build\tests\Release\run_combined_tests.exe
```
