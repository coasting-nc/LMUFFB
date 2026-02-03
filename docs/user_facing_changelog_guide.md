# User-Facing Changelog Guide

## Purpose

This document provides guidelines for creating entries in the **User-Facing Changelog** (`USER_CHANGELOG.md`). This changelog is designed for end users and uses **BBCode formatting** for easy copy-paste to forums. It emphasizes understandable, benefit-focused release notes, while the technical `CHANGELOG_DEV.md` (in Markdown) serves developers and power users.

## Format: BBCode for Forums

The user-facing changelog uses BBCode syntax because entries are posted directly to forum threads. BBCode is the standard markup language for most forum software.

### BBCode vs Markdown Quick Reference

| Element | Markdown | BBCode |
|---------|----------|--------|
| Bold | `**text**` | `[b]text[/b]` |
| Italic | `*text*` | `[i]text[/i]` |
| Link | `[text](url)` | `[url=url]text[/url]` or just `url` |
| Header | `## Text` | `[size=5][b]Text[/b][/size]` |
| Bullet List | `* item` | `[list][*]item[/list]` |
| Code/Mono | `` `code` `` | `[font=courier]code[/font]` or `[code]code[/code]` |

## Core Principles

### 1. User-First Language
- **Avoid**: Technical jargon, implementation details, class names, file paths
- **Use**: Clear descriptions of what changed from the user's perspective
- **Example**:
  - ❌ "Refactored `FFBEngine::calculate_force` with context-based processing"
  - ✅ "Improved performance and stability of force feedback calculations"

### 2. Focus on Benefits
Explain HOW the change helps the user, not just WHAT changed.

**Example**:
- ❌ "Added Auto-Connect to LMU"
- ✅ "**Auto-Connect to LMU**: The app now automatically connects every 2 seconds, eliminating the need to click 'Retry' manually. Status shows 'Connecting...' in yellow while searching and 'Connected' in green when active."

### 3user Relevance First
Include changes that users will:
- **Notice** during normal use
- **Configure** via UI controls
- **Benefit from** (fixes, improvements, new features)

**Exclude** or minimize:
- Internal refactoring (unless it improves performance/stability noticeably)
- Test suite additions
- Developer documentation updates
- Code quality improvements (unless they fix bugs users encountered)

### 4. Be Concise
- Keep entries brief but informative
- Use bullet points for multiple changes
- Group related changes together
- Limit to 2-4 sentences per feature

## Entry Structure

### Version Header (BBCode Format)
```bbcode
[size=5][b]Date[/b][/size]
[b]Version X.Y.Z - Feature Name[/b]

[b]New release[/b] (X.Y.Z): https://github.com/coasting-nc/LMUFFB/releases
```

**Example**:
```bbcode
[size=5][b]January 31, 2026[/b][/size]
[b]Version 0.6.39 - Auto-Connect & Performance[/b]

[b]Special Thanks[/b] to [b]@AndersHogqvist[/b] for the Auto-connect feature!

[b]New release[/b] (0.6.39): https://github.com/coasting-nc/LMUFFB/releases
```

**Formatting Requirements**:
- **Date header**: Use `[size=5][b]Date[/b][/size]` for main heading
- **Version title**: Use `[b]Version X.Y.Z - Feature Name[/b]`
- **"New release"**: Always bold: `[b]New release[/b]`
- **Version in link**: Include version in parentheses after "New release"
- **Links**: Just paste the URL directly (BBCode auto-links) or use `[url=url]text[/url]`
- **Attribution**: Use `[b]@Username[/b]` for contributor mentions

### Change Categories

Use these categories in order of user impact:

1. **Added** - New features users can see/use
2. **Fixed** - Bug fixes that improve user experience
3. **Changed** - Modified behavior users will notice
4. **Improved** - Performance/quality enhancements

### Entry Format (BBCode)

```bbcode
[b]Category[/b]
[list]
[*][b]Feature Name[/b]: 1-2 sentence description focusing on user benefit
[/list]
```

**For multiple items**, use nested lists:
```bbcode
[b]Added[/b]
[list]
[*][b]Feature One[/b]: Description here
[*][b]Feature Two[/b]: Description here
[/list]
```

## Writing Guidelines

### DO ✅

- **Start with the benefit**: "Fixed vibrations when stationary" instead of "Implemented speed gate"
- **Mention UI locations**: "Added new sliders in Advanced Settings"
- **Include defaults**: "Now defaults to 18 km/h (previously 10 km/h)"
- **Provide context**: "This fixes the shaking wheel in the pits"
- **Group related items**: Combine multiple slider additions into one entry
- **Use bold for names**: `[b]Auto-Connect[/b]`, `[b]Speed Gate[/b]`, `[b]ABS Pulse[/b]`
- **Use lists for multiple items**: Use BBCode `[list][*]item[/list]` format

### DON'T ❌

- **List every test added**: Users don't care about `test_speed_gate_custom_thresholds()`
- **Quote code**: Avoid `m_prev_vert_accel` or `FFBEngine.h`
- **Over-explain internals**: "Uses std::fmod for phase wrapping" → "Fixed stuttering during vibration effects"
- **Include file paths**: `src/lmu_sm_interface/SafeSharedMemoryLock.h`
- **List every config parameter**: Just mention the feature, not implementation details
- **Use Markdown syntax**: Use BBCode format `[b]bold[/b]` not `**bold**`

## Length Guidelines

- **Major version** (0.x.0): 4-8 bullet points
- **Minor version** (0.6.x): 2-6 bullet points
- **Patch version** (0.6.x, bug fix only): 1-3 bullet points

**Target**: 50-150 words per version entry

## Examples

### Good Entry ✅

```bbcode
[size=5][b]December 28, 2025[/b][/size]
[b]Version 0.6.22 - Vibration Fixes[/b]

[b]New release[/b] (0.6.22): https://github.com/coasting-nc/LMUFFB/releases

Fixed vibrations when car still / in the pits:
[list]
[*]Disabled vibration effects when speed below a certain threshold (ramp from 0.0 vibrations at < 0.5 m/s to 1.0 vibrations at > 2.0 m/s).
[*]Automatic Idle Smoothing: Steering Torque is smoothed when car is stationary or moving slowly (< 3.0 m/s). This should remove any remaining engine vibrations.
[*]Road Texture Fallback: alternative method to calculate Road Texture when data is encrypted. You can now feel bumps and curbs on DLC cars.
[/list]
```

**Why it's good**:
- Focuses on user-facing problem ("wheel shaking in pits")
- Explains the solution clearly
- No code or technical details
- Uses BBCode lists for readability
- Mentions what users will notice ("smoothed", "feel bumps")

### Bad Entry ❌

```bbcode
[size=5][b]December 28, 2025[/b][/size]
[b]Version 0.6.22 - Technical Updates[/b]

[b]Added[/b]
[list]
[*]Implemented a dynamic Low Pass Filter (LPF) for the steering shaft torque with automatic smoothing at car speed < 3.0 m/s
[*]Road texture fallback mechanism using mLocalAccel.y when mVerticalTireDeflection is missing
[*]test_stationary_gate() and updated test_missing_telemetry_warnings()
[/list]

[b]Changed[/b]
[list]
[*]Updated warning to include "(Likely Encrypted/DLC Content)"
[/list]
```

**Why it's bad**:
- Technical jargon ("LPF", "mLocalAccel.y")
- Lists test additions (irrelevant to users)
- Doesn't explain user benefit
- No mention of what problem it solves

## Special Cases

### Breaking Changes
Clearly mark and explain migration:

```bbcode
[b]Changed[/b]
[list]
[*][b]⚠️ BREAKING: Understeer Effect Range[/b]: The slider now uses 0.0-2.0 instead of 0-200. Old values are automatically converted (50.0 → 0.5). See new scale guide in tooltip.
[/list]
```

### Community Contributions
Always credit:

```bbcode
[b]Special Thanks[/b] to [b]@DiSHTiX[/b] for the icon implementation!
```

### Critical Fixes
Use emphasis:

```bbcode
[b]Fixed[/b]
[list]
[*][b]CRITICAL[/b]: Fixed wheel staying locked at last force when pausing the game, which could cause injury. The wheel now immediately releases when entering menus.
[/list]
```

## Review Checklist

Before finalizing an entry, ask:

- [ ] Would a non-technical user understand this?
- [ ] Have I explained the benefit, not just the change?
- [ ] Is it concise (no walls of text)?
- [ ] Did I avoid code/file references?
- [ ] Did I group related changes?
- [ ] Is it something users will actually notice?
- [ ] Am I using BBCode formatting correctly?
- [ ] Did I use `[b]bold[/b]` for emphasis, not `**bold**`?
- [ ] Did I use `[list][*]item[/list]` for lists?

## Versioning Strategy

### When to Include in User-Facing Changelog

| Change Type | Include? | Rationale |
|-------------|----------|-----------|
| New GUI feature/slider | ✅ Yes | Users see and use it |
| Bug fix (user-reported) | ✅ Yes | Solves user pain point |
| Performance improvement (noticeable) | ✅ Yes | Users feel the difference |
| New preset | ✅ Yes | Users can select it |
| Refactoring (no behavior change) | ❌ No | Users won't notice |
| Test additions | ❌ No | Developer-only |
| Documentation (dev docs) | ❌ No | Not user-facing |
| Code review fixes | ❌ Maybe | Only if it fixes a user-visible bug |

## Version Entry Template (BBCode)

```bbcode
[size=5][b][Month Day, Year][/b][/size]
[b]Version X.Y.Z - [Feature Name][/b]

[b]Special Thanks[/b] to [b]@Contributors[/b] for [Contribution]!

[b]New release[/b] (X.Y.Z): https://github.com/coasting-nc/LMUFFB/releases

[b]Added[/b]
[list]
[*][b]Feature Name[/b]: User benefit description
[/list]

[b]Fixed[/b]
[list]
[*][b]Issue Description[/b]: What was wrong and how it's now better
[/list]

[b]Changed[/b]
[list]
[*][b]Feature Name[/b]: What changed and why users care
[/list]

[b]Improved[/b]
[list]
[*][b]Area[/b]: Performance/quality improvement users will feel
[/list]
```

---

## Conversion Example: Technical → User-Facing

**From CHANGELOG_DEV.md** (Technical, Markdown):
```markdown
### Refactored
- **GameConnector Lifecycle**:
  - Introduced `Disconnect()` method to centralize resource cleanup
  - Fixed potential resource leaks in `TryConnect()` 
  - Updated `IsConnected()` with double-checked locking pattern
  - Process Handle Robustness: Connection succeeds even if window handle unavailable
  - Updated destructor to ensure handles properly closed
```

**To User-Facing Changelog** (BBCode):
```bbcode
[b]Improved[/b]
[list]
[*][b]Connection Reliability[/b]: Fixed connection issues and resource leaks that could cause the app to not detect the game properly. Connection is now more robust even if the game window isn't fully loaded yet.
[/list]
```

**Changes made**:
- Removed method names and technical details
- Focused on user benefit: "more reliable connection"
- Combined multiple technical points into one user-facing benefit
- Explained in terms users understand: "game not detected properly"
- Converted from Markdown to BBCode format

