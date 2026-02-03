# Proposed Alternative Names for User-Facing Changelog

## Current Name
`Version Releases (user facing changelog).md`

## Issues with Current Name
- Contains parentheses (which can cause issues in some tools/shells)
- Contains spaces (requires quoting in command-line operations)
- Verbose and awkward
- Not immediately clear what "Version Releases" means
- Inconsistent with standard naming conventions

## Proposed Alternatives (Recommended Order)

### 1. `RELEASES.md` ⭐ **RECOMMENDED**
**Pros:**
- Short, clear, and conventional
- Widely recognized in open source projects
- No spaces or special characters
- Pairs well with `CHANGELOG.md` (both caps, similar length)
- Standard practice in many GitHub projects

**Cons:**
- Some projects use RELEASES for binaries download page

**Example peers**: Docker, Kubernetes, many GitHub projects

---

### 2. `RELEASE_NOTES.md` ⭐ **STRONG ALTERNATIVE**
**Pros:**
- Very clear what it contains
- Standard format used by Microsoft, Apple, and enterprise software
- No ambiguity
- Professional tone

**Cons:**
- Slightly longer
- Underscore instead of space

**Example peers**: Visual Studio Code, .NET, Windows

---

### 3. `NEWS.md`
**Pros:**
- Very short and simple
- Common in GNU/Linux projects
- No special characters
- Easy to type

**Cons:**
- May imply blog posts or announcements rather than version history
- Less explicit about content

**Example peers**: GNU tools, many Linux utilities

---

### 4. `HISTORY.md`
**Pros:**
- Clear indication it's historical record
- Simple, no special characters
- Good for user-facing summaries

**Cons:**
- Sometimes used for full technical changelog
- Could be confused with git history

**Example peers**: Some older open source projects

---

### 5. `WHATS_NEW.md`
**Pros:**
- Very user-friendly language
- Clear this is about new features/changes
- Commonly used in consumer software

**Cons:**
- Implies only latest version, not full history
- Two words makes it slightly awkward
- Less conventional in open source

**Example peers**: Mobile apps, commercial software

---

### 6. `USER_CHANGELOG.md`
**Pros:**
- Explicitly differentiates from technical CHANGELOG.md
- Clear target audience
- Matches existing naming pattern

**Cons:**
- Redundant if technical changelog is renamed to CHANGELOG_DEV.md
- Slightly awkward compound name

---

### 7. `UPDATES.md`
**Pros:**
- Simple and clear
- User-friendly language
- No special characters

**Cons:**
- Could be confused with update mechanism/scripts
- Less standard

---

## Recommendation

### **Primary Recommendation: `RELEASES.md`**

**Rationale:**
1. **Industry standard**: Widely used in GitHub projects for user-facing release notes
2. **Clean naming**: No spaces, special characters, all caps matches CHANGELOG.md
3. **Clear purpose**: Immediately recognizable to users
4. **Short and memorable**: Easy to reference in documentation
5. **SEO-friendly**: "Releases" is what users search for

### **Secondary Recommendation: `RELEASE_NOTES.md`** 

Use this if:
- You want to be extra explicit about the content
- Your user base is more enterprise/professional
- You prefer the Microsoft/Apple convention

## Implementation Impact

### Files to Update if Renamed

If renaming from `Version Releases (user facing changelog).md` to `RELEASES.md`:

1. **README.md** - Update any references to the changelog
2. **.agent/workflows/create-new-version.md** - Update workflow instructions
3. **GitHub Release Templates** (if any) - Update links
4. **Documentation** - Any developer/contributor guides
5. **Issue Templates** (if any) - Update references

### Git History

**Option 1: Rename with git mv (preserves history)**
```bash
git mv "Version Releases (user facing changelog).md" RELEASES.md
```

**Option 2: Create new file and redirect old**
- Create `RELEASES.md` with current content
- Replace old file content with: "This file has been renamed to RELEASES.md"

**Recommended**: Option 1 (git mv) to preserve history

## Summary Table

| Name | Length | Special Chars | Clarity | Convention | Recommendation |
|------|--------|---------------|---------|------------|----------------|
| `RELEASES.md` | ★★★ | ✅ None | ★★★ | ★★★ | ⭐⭐⭐ |
| `RELEASE_NOTES.md` | ★★☆ | ⚠️ Underscore | ★★★ | ★★★ | ⭐⭐ |
| `NEWS.md` | ★★★ | ✅ None | ★★☆ | ★★☆ | ⭐ |
| `HISTORY.md` | ★★★ | ✅ None | ★★☆ | ★☆☆ | ⭐ |
| `WHATS_NEW.md` | ★★☆ | ⚠️ Underscore | ★★★ | ★☆☆ | - |
| `USER_CHANGELOG.md` | ★☆☆ | ⚠️ Underscore | ★★★ | ★☆☆ | - |

**Legend:**
- ★★★ = Excellent
- ★★☆ = Good
- ★☆☆ = Acceptable
- ⭐ = Recommendation level (more stars = stronger recommendation)
