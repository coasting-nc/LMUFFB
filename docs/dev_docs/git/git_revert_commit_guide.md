# Git Commit Reversal Guide

## Commit to Reverse
**Commit ID:** `01efb20640861548d3bb916a1013f83cae7c298b`

---

## Options for Reversing and Rewriting Git History

### Option 1: Remove the commit entirely (if it's the last commit)

```bash
git reset --hard HEAD~1
```

**Explanation:** This removes the last commit from your branch history and discards all changes from that commit. The `--hard` flag resets both the staging area and working directory to match the previous commit.

⚠️ **Warning:** This permanently deletes the changes from that commit.

---

### Option 2: Remove the commit but keep the changes (if it's the last commit)

```bash
git reset --soft HEAD~1
```

**Explanation:** This removes the last commit from history but keeps all the changes staged (ready to commit again). Useful if you want to modify the commit or split it into multiple commits.

---

### Option 3: Remove the commit and keep changes unstaged

```bash
git reset --mixed HEAD~1
# or simply
git reset HEAD~1
```

**Explanation:** This removes the last commit and keeps the changes in your working directory but unstaged. `--mixed` is the default behavior of `git reset`.

---

### Option 4: Interactive rebase (if it's not the last commit)

```bash
git rebase -i 01efb20640861548d3bb916a1013f83cae7c298b~1
```

**Explanation:** This opens an interactive rebase editor. You can then mark the commit with `drop` (or just delete the line) to remove it from history. This is useful when the commit is buried in your history.

---

### Option 5: Revert to a specific commit using its hash

```bash
git reset --hard 01efb20640861548d3bb916a1013f83cae7c298b~1
```

**Explanation:** This resets your branch to the commit immediately before `01efb206...`. The `~1` means "one commit before this one."

---

## Force Pushing After Rewriting History

After any of these operations, if you've already pushed the commit to a remote repository, you'll need to force push:

```bash
git push --force
# or safer:
git push --force-with-lease
```

**Explanation:** 
- `--force` overwrites the remote history with your local history
- `--force-with-lease` is safer—it only force pushes if no one else has pushed changes to the remote branch since your last fetch

⚠️ **Warning:** Force pushing rewrites history on the remote. Only do this if:
- You're working on a personal branch, OR
- You've coordinated with your team and they're aware

---

## Which Option Should You Use?

- **If it's your last commit and you want to completely remove it:** Use Option 1
- **If it's your last commit and you want to modify it:** Use Option 2
- **If the commit is somewhere in the middle of your history:** Use Option 4
- **If you want to be specific about which commit to reset to:** Use Option 5

---

## Additional Notes

### Checking Your Git History

Before reverting, you can check your commit history:

```bash
git log --oneline -10
```

This shows the last 10 commits in a compact format.

### Verifying the Commit to Revert

To see details about the specific commit:

```bash
git show 01efb20640861548d3bb916a1013f83cae7c298b
```

### Creating a Backup Branch (Recommended)

Before rewriting history, create a backup branch:

```bash
git branch backup-before-revert
```

This allows you to recover if something goes wrong.

---

**Document Created:** 2025-12-16  
**Purpose:** Guide for reversing commit `01efb20640861548d3bb916a1013f83cae7c298b`
