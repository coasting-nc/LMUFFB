
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

# squash branch and merge into main

## Option 1
squash branch commits into one:
git reset --soft $(git merge-base main HEAD)
(This moves your branch pointer back to the moment you branched off main, but keeps all your work staged in the index.)
Commit the single "squashed" result:
git commit -m "Your combined commit message"


## Option 2
alternative:
git checkout main
git merge --squash feature-branch
git commit -m "Features from branch"

(This is the "simple" way to merge, but it doesn't "clean up" the history of the original branch.)

# Rebase approach

git checkout main
git pull origin main


git checkout your-feature-branch
git rebase main

git rebase --continue

git push origin your-feature-branch --force-with-lease

## Explanation of the git rebase approach

To use the rebase approach to keep your feature branch up to date with

`main`, follow these steps.

### **The Rebase Workflow**

This replaces the "back-merge" (

`git merge main`) in your feature branch.

#### **1\. Update your local main**

First, ensure you have the latest changes from the server.

powershell
`git checkout main`
`git pull origin main`

#### **2\. Rebase your feature branch**

Move back to your feature branch and "replay" your work on top of the new

`main`.
powershell
`git checkout your-feature-branch`
`git rebase main`

#### **3\. Handle Conflicts (If any)**

If Git stops because of a conflict, the process pauses. You then:

1. Open the files and fix the conflicts.
2. Stage the fixed files: `git add <file>`
3. Continue the rebase:
4. powershell
5. `git rebase --continue`
6. *(Note: You do not run `git commit` during a rebase; `--continue` handles it for you.)*

#### **4\. Update the Remote (The "Force Push")**

Because

`rebase` rewrites the history of your branch, if you have already pushed this branch to the remote (GitHub/GitLab), a simple `git push` will be rejected. You must "force" update it:
powershell
`git push origin your-feature-branch --force-with-lease`
*Tip:*

*`--force-with-lease` is safer than `--force` because it won't overwrite the remote if someone else has added new commits to that branch in the meantime.*
---

### **Comparison: Rebase vs. Merge**

| Feature | Git Merge (Back-merge) | Git Rebase |
| :---- | :---- | :---- |
| History | Preserves exact chronological order. | Creates a clean, linear history. |
| Log Appearance | Includes "Merge branch 'main' into..." commits. | Looks like you started your work *after* the latest main. |
| Conflict Resolution | You resolve all conflicts once at the end. | You might resolve conflicts commit-by-commit. |
| Remote Safety | Safe to use standard `git push`. | Requires Force Push if branch was already pushed. |

Rule of Thumb:

* Use Rebase for your own private feature branches to keep the project history beautiful.
* Never Rebase a branch that other people are also actively working on (shared branches), as the force push will break their local copies.


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


