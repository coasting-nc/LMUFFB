---
name: versioning_and_changelog
description: Guidelines for incrementing project version and maintaining dual changelogs
---

# Versioning and Changelog

Use this skill when completing a task to ensure the project version is correctly incremented and changes are documented.

## Version Increment Rule

- **Smallest Increment Only**: Always add **+1 to the rightmost number** in the version string.
- **Example**: `0.6.39` → `0.6.40`
- **Example**: `0.7.0` → `0.7.1`
- **Constraint**: Do NOT increment minor or major versions unless explicitly instructed by the user.

### Files to Update
1. `VERSION` (root directory)

## Dual Changelogs

### 1. Developer Changelog (`CHANGELOG_DEV.md`)
- Add a concise entry under the new version number.
- Focus on technical changes, bug fixes, and architecture updates.

### 2. User Changelog (`USER_CHANGELOG.md`)
- Add user-friendly descriptions of new features or visible improvements.
- Avoid overly technical jargon.

## Workflow Summary
1. Determine the new version number.
2. Update `VERSION` 
3. Update `CHANGELOG_DEV.md`.
4. Update `USER_CHANGELOG.md`.
