# Steps to Merge into Main:

git checkout main
git pull origin main
git merge <branch name>
git push origin main

Optionally, delete the feature branch locally (after successful merge):
git branch -d <branch name>

And delete it from remote if it exists there:
git push origin --delete <branch name>


