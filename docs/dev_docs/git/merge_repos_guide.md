# Merging from another repo into coasting-nc/LMUFFB

## Overview
This guide explains how to merge changes from remote repo into `https://github.com/coasting-nc/LMUFFB` as a single squashed commit, removing all author information from the remote repo.

## Prerequisites
- You need write access to `https://github.com/coasting-nc/LMUFFB`
- Git configured with coasting-nc credentials

## Step-by-Step Process

### 1. Clone the coasting-nc repository (if not already cloned)
```bash
cd c:\dev\personal2
git clone https://github.com/coasting-nc/LMUFFB LMUFFB-coasting
cd LMUFFB-coasting
```

### 2. Ensure you're on the main branch and it's up to date
```bash
git checkout main
git pull origin main
```

### 3. Add remote repo as a remote
```bash
git remote add <alias remote repo> <remote repo>
git fetch <remote repo>
```

### 4. Create a temporary branch for the merge
```bash
git checkout -b temp-merge-mauro
```

### 5. Merge remote repo into the temporary branch
```bash
git merge <remote repo>/main --allow-unrelated-histories
```
**Note:** Resolve any merge conflicts if they occur.

### 6. Switch back to main and perform a squash merge
```bash
git checkout main
git merge --squash temp-merge-mauro
```

### 7. Commit the squashed changes with coasting-nc as author
```bash
git commit -m "Merge changes from remote repo

Squashed merge of all changes from the remote repo fork." --author="coasting-nc <coasting-nc@users.noreply.github.com>"
```
**Note:** Replace the email with the actual coasting-nc email if known.

### 8. Remove the remote repo remote
```bash
git remote remove remote repo
```

### 9. Delete the temporary branch
```bash
git branch -d temp-merge-remote-repo
```

### 10. Push the changes to coasting-nc/LMUFFB
```bash
git push origin main
```

## Verification

After completing these steps:
- Check `git remote -v` to verify remote repo is removed
- Check `git log` to verify the squashed commit shows coasting-nc as author
- Verify the changes are pushed to `https://github.com/coasting-nc/LMUFFB`

## Alternative: Using git rebase for cleaner history

If you want even more control over the commit message and ensure no traces:

```bash
# After step 5 (while on temp-merge-mauro)
git checkout main
git merge --squash temp-merge-mauro
git commit --author="coasting-nc <coasting-nc@users.noreply.github.com>"
# Then continue with steps 8-10
```

## Important Notes

1. **Backup First:** Consider creating a backup of both repositories before proceeding
2. **Conflicts:** If merge conflicts occur in step 5, you'll need to resolve them manually
3. **Author Information:** The `--author` flag ensures coasting-nc is listed as the commit author
4. **Remote Removal:** Step 8 only removes the remote from your local repository; it doesn't affect the actual GitHub repositories
5. **Force Push:** If you need to force push (not recommended unless necessary): `git push origin main --force`
