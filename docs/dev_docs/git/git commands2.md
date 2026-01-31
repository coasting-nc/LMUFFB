
# Steps to Merge into Main:

git checkout main
git pull origin main
git merge <branch name>
git push origin main

Optionally, delete the feature branch locally (after successful merge):
git branch -d <branch name>

And delete it from remote if it exists there:
git push origin --delete <branch name>

# Git Commands for Merging a PR into Main

## 1. Merge into Main
Once `pr-19` is clean and committed:

```powershell
# Switch to the main branch
git checkout main

# Ensure main is up to date
git pull origin main

# Merge the feature branch (pr-19) into main
git merge pr-19

# Push the updated main branch to the remote repository
git push origin main
```

## 3. (Optional) Cleanup
After successfully pushing `main`:

```powershell
# Delete the local feature branch
git branch -d pr-19
```