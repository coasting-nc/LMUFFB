---
description: How to create a new version release with dual changelogs
---

# Create New Version Release

This workflow guides you through creating a new version release with proper changelog entries in both technical and user-facing formats.

## Prerequisites

- All code changes for the version are complete
- You have reviewed the changes to document
- You know the version number (e.g., 0.6.40)

## Steps

### 1. Update VERSION File

Update the `VERSION` file with the new semantic version number:

```
0.6.40
```

### 2. Update src/Version.h

Update the version constant in `src/Version.h`:

```cpp
constexpr const char* VERSION = "0.6.40";
```

### 3. Add Entry to CHANGELOG_DEV.md (Technical)

Add a new detailed technical entry at the TOP of `CHANGELOG_DEV.md` following this structure:

```markdown
## [0.6.40] - YYYY-MM-DD

### Added
- **Feature Name**: Detailed technical description
  - Implementation details
  - Technical specifications
  - Code-level changes and their impact
  
### Fixed
- **Issue Description**: Technical explanation of bug and fix
  - Root cause analysis
  - Solution implemented
  - Affected components/files
  
### Changed
- **Component Name**: What changed technically
  - API changes
  - Behavior modifications
  - Migration notes if needed
  
### Refactored
- **Code Section**: Refactoring details
  - Architectural improvements
  - Code quality enhancements
  - Performance optimizations

### Documentation
- List documentation updates
- Developer guides added/updated
- Code review summaries

### Testing
- Test suite additions
- Test coverage improvements
- Regression tests added
```

**Guidelines for CHANGELOG_DEV.md**:
- Be DETAILED and TECHNICAL
- Include implementation specifics
- List file paths, method names, technical terms
- Document breaking changes clearly
- Include migration notes for developers
- List all test additions
- Reference code reviews and implementation docs
- Target audience: developers, contributors, power users

### 4. Add Entry to User-Facing Changelog

Add a new USER-FRIENDLY entry in **BBCode format** to `USER_CHANGELOG.md`.

**IMPORTANT**: Read `docs/user_facing_changelog_guide.md` FIRST for detailed guidelines.

**Format: BBCode** (for forum posting)

Structure:

```bbcode
[size=5][b]Month Day, Year[/b][/size]
[b]Version 0.6.40 - Feature Name[/b]

[b]Special Thanks[/b] to [b]@contributors[/b] for [contribution]!

[b]New release[/b] (0.6.40): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Feature Name[/b]: Brief, benefit-focused description of what users will notice or can now do
[/list]

[b]Fixed[/b]
[list]
[*][b]Issue Description[/b]: What was wrong and how it's better now, in plain language
[/list]

[b]Changed[/b]
[list]
[*][b]Feature Name[/b]: What changed from the user's perspective and why they should care
[/list]

[b]Improved[/b]
[list]
[*][b]Area[/b]: Performance or quality improvement users will feel
[/list]
```

**Formatting Requirements**:
- **"New release" text**: ALWAYS use BBCode bold: `[b]New release[/b]`
- **Version in link**: Include version number in parentheses after "New release"
- **Release link**: Always include link to GitHub releases page
- **Headers**: Use `[size=5][b]Date[/b][/size]` for main heading
- **Bold**: Use `[b]text[/b]` NOT `**text**`
- **Lists**: Use `[list][*]item[/list]` NOT `* item`
- **Links**: Just paste URL or use `[url=url]text[/url]`

**Key Differences from Technical Changelog**:
- **Brevity**: 2-4 sentences per item, not paragraphs
- **Plain Language**: No code, file paths, or jargon
- **User Focus**: What they see/feel, not how it works
- **Benefits**: Explain WHY it matters, not just WHAT changed
- **Selective**: Skip refactoring, tests, dev docs unless they fix user-visible bugs

**Conversion Tips**:
- "Refactored FFBEngine with context-based processing" → "Improved performance and stability of force calculations"
- "Added test_speed_gate_custom_thresholds()" → [Skip this entirely]
- "Fixed mVerticalTireDeflection fallback using mLocalAccel.y" → "Fixed Road Texture on encrypted DLC cars"

### 5. Review Both Entries

**Technical Changelog (CHANGELOG_DEV.md) Check**:
- [ ] All significant code changes documented
- [ ] Implementation details included
- [ ] Breaking changes clearly marked
- [ ] Test additions listed
- [ ] Documentation updates listed
- [ ] Technical terminology used appropriately

**User-Facing Changelog Check**:
- [ ] Non-technical language used
- [ ] User benefits clear
- [ ] Concise (50-150 words)
- [ ] No code/file references
- [ ] Explains WHAT CHANGED from user perspective
- [ ] Skip items users won't notice (tests, refactoring, dev docs)
- [ ] BBCode formatting used (not Markdown)
- [ ] Bold uses `[b]text[/b]` NOT `**text**`
- [ ] Lists use `[list][*]item[/list]` NOT `* item`

### 6. Build and Test

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release --clean-first
```

Then run tests:

```powershell
.\build\tests\Release\run_combined_tests.exe
```

Ensure all tests pass before proceeding.

### 7. Commit Changes (DO NOT STAGE/PUSH)

**IMPORTANT**: Following user rules, you MUST NOT use `git add`, `git commit`, or `git push`.

Review your changes and inform the user that the following files have been modified:
- `VERSION`
- `src/Version.h`
- `CHANGELOG_DEV.md`
- `USER_CHANGELOG.md`

The user will review and commit these changes manually.

## Example: Complete Version Creation

**Scenario**: Adding configurable speed gate thresholds

### VERSION
```
0.6.23
```

### CHANGELOG_DEV.md (Technical)
```markdown
## [0.6.23] - 2025-12-28

### Added
- **Configurable Speed Gate**:
  - Introduced the "Stationary Vibration Gate" in Advanced Settings
  - Added "Mute Below" (0-20 km/h) and "Full Above" (1-50 km/h) sliders
  - Implemented safety clamping to ensure upper >= lower threshold
  - Updated automatic idle smoothing to use user-configured thresholds with 3.0 m/s safety floor
- **Advanced Physics Configuration**:
  - Added support for `road_fallback_scale` and `understeer_affects_sop` in Preset system
  - Integrated parameters into FFB engine configuration
- **Improved Test Coverage**:
  - Added `test_speed_gate_custom_thresholds()` to verify dynamic threshold scaling
  - Updated `test_stationary_gate()` to align with new 5.0 m/s default

### Changed
- **Default Speed Gate**: Increased from 10.0 km/h to **18.0 km/h (5.0 m/s)**
  - Eliminates violent engine vibrations in LMU/rF2 below 15 km/h by default
  - Provides surgical smoothing without affecting driving feel
```

### User-Facing Changelog (BBCode)
```bbcode
[size=5][b]December 28, 2025[/b][/size]
[b]Version 0.6.23 - Smoothing Adjustments[/b]

[b]New release[/b] (0.6.23): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Customizable Low-Speed Smoothing[/b]: Added adjustable sliders in Advanced Settings to control when vibrations fade out at low speeds. Default increased to 18 km/h to better eliminate engine rumble when stationary or in pits.
[/list]

[b]Improved[/b]
[list]
[*][b]Smoother Pit Experience[/b]: Fixed remaining vibrations at very low speeds by automatically applying smoothing below 18 km/h instead of the previous 10 km/h.
[/list]
```

## Notes

- **Order matters**: CHANGELOG_DEV.md entry goes at TOP of the file (newest first)
- **Date format**: Use YYYY-MM-DD for CHANGELOG_DEV.md, "Month Day, Year" for user-facing
- **Special Thanks**: Include community contributors when applicable
- **Both required**: EVERY version needs entries in BOTH changelogs (with appropriate detail levels)
- **Consistency**: Version numbers must match across VERSION, Version.h, and both changelogs
