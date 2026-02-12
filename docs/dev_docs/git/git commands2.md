
# Steps to Merge into Main:

git checkout main
git pull origin main
git merge <branch name>
git push origin main

Optionally, delete the feature branch locally (after successful merge):
git branch -d <branch name>

And delete it from remote if it exists there:
git push origin --delete <branch name>

# Other commands

Update your remote references:
git fetch origin

git merge --abort
git merge -X theirs <branch name>

# More info

Is "Merging Main into Feature" a good practice?

Yes, it is a very common and recommended way to proceed. It is often called a "back-merge." It allows you to:

* Resolve conflicts in the safety of your feature branch without affecting the stable main.
* Ensure your new code is compatible with the latest updates from other developers.
* Run tests against the combined code before the final integration.

The "Better" Way: Git Rebase Many professional teams prefer `git rebase main` while on the feature branch instead of merging.

How it works: It takes your feature commits, "lifts" them up, updates your branch base to the latest main, and then reapplies your commits on top.
Result: You get a perfectly linear history without "Merge branch 'main' into..." commits cluttering the log.

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

# Update the PR Branch and Let GitHub Auto-Close

## Switch to the PR branch
git checkout feature/auto-connect-lmu

## Reset it to match main (all your changes are already in main)
git reset --hard main

## Force push to update the remote PR branch
git push origin feature/auto-connect-lmu --force

## Switch back to main
git checkout main


