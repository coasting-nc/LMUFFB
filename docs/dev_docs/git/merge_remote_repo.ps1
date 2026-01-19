#!/usr/bin/env pwsh
param(
    [Parameter(Mandatory=$true)]
    [string]$RemoteRepoUrl,
    [Parameter(Mandatory=$false)]
    [string]$AccessToken = "",
    [Parameter(Mandatory=$false)]
    [string]$RemoteAlias = "temp-remote",
    [Parameter(Mandatory=$false)]
    [string]$AuthorEmail = "coasting-nc@users.noreply.github.com"
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "`n==> $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host " $Message" -ForegroundColor Green
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-Host " $Message" -ForegroundColor Yellow
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host " $Message" -ForegroundColor Red
}

Write-Step "Verifying git repository..."
try {
    $gitCheck = git rev-parse --is-inside-work-tree 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Not a git repository" }
    Write-Success "Git repository confirmed"
} catch {
    Write-Error-Custom "This script must be run from within a git repository"
    exit 1
}

Write-Step "Checking current branch..."
$currentBranch = git branch --show-current
if ($currentBranch -ne "main") {
    Write-Warning-Custom "Current branch is '$currentBranch', not 'main'"
    $response = Read-Host "Do you want to switch to main? (y/n)"
    if ($response -eq 'y') {
        git checkout main
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "Failed to checkout main branch"
            exit 1
        }
    } else {
        Write-Error-Custom "Script must be run from main branch"
        exit 1
    }
}
Write-Success "On main branch"

Write-Step "Updating main branch from origin..."
git pull origin main
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to pull latest changes from origin/main"
    exit 1
}
Write-Success "Main branch updated"

Write-Step "Checking for existing remote '$RemoteAlias'..."
$existingRemote = git remote | Where-Object { $_ -eq $RemoteAlias }
if ($existingRemote) {
    Write-Warning-Custom "Remote '$RemoteAlias' already exists, removing it..."
    git remote remove $RemoteAlias
    Write-Success "Existing remote removed"
}

# Build the remote URL with access token if provided
$remoteUrlWithAuth = $RemoteRepoUrl
if ($AccessToken) {
    Write-Step "Adding access token to remote URL..."
    if ($RemoteRepoUrl -match '^https://github\.com/(.+)$') {
        $remoteUrlWithAuth = "https://$AccessToken@github.com/$($Matches[1])"
        Write-Success "Access token added"
    } else {
        Write-Warning-Custom "Could not parse GitHub URL, using URL as-is"
    }
}

Write-Step "Adding remote repository as '$RemoteAlias'..."
git remote add $RemoteAlias $remoteUrlWithAuth
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to add remote repository"
    exit 1
}
Write-Success "Remote repository added"

Write-Step "Fetching from remote repository..."
git fetch $RemoteAlias
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to fetch from remote repository"
    git remote remove $RemoteAlias
    exit 1
}
Write-Success "Remote repository fetched"

$tempBranch = "temp-merge-$(Get-Date -Format 'yyyyMMddHHmmss')"
Write-Step "Creating temporary branch '$tempBranch'..."
git checkout -b $tempBranch
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to create temporary branch"
    git remote remove $RemoteAlias
    exit 1
}
Write-Success "Temporary branch created"

Write-Step "Merging $RemoteAlias/main into temporary branch..."
Write-Warning-Custom "If merge conflicts occur, you will need to resolve them manually"
git merge "$RemoteAlias/main" --allow-unrelated-histories
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Merge failed or has conflicts"
    Write-Host "`nTo resolve conflicts:" -ForegroundColor Yellow
    Write-Host "1. Resolve conflicts in the affected files" -ForegroundColor Yellow
    Write-Host "2. Stage resolved files: git add FILENAME" -ForegroundColor Yellow
    Write-Host "3. Complete the merge: git commit" -ForegroundColor Yellow
    Write-Host "4. Re-run this script OR continue manually" -ForegroundColor Yellow
    Write-Host "`nTo abort:" -ForegroundColor Yellow
    Write-Host "git merge --abort" -ForegroundColor Yellow
    Write-Host "git checkout main" -ForegroundColor Yellow
    Write-Host "git branch -D $tempBranch" -ForegroundColor Yellow
    Write-Host "git remote remove $RemoteAlias" -ForegroundColor Yellow
    exit 1
}
Write-Success "Merge completed successfully"

Write-Step "Switching back to main branch..."
git checkout main
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to checkout main branch"
    exit 1
}
Write-Success "Switched to main"

Write-Step "Performing squash merge..."
git merge --squash $tempBranch
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Squash merge failed"
    git branch -D $tempBranch
    git remote remove $RemoteAlias
    exit 1
}
Write-Success "Squash merge completed"

Write-Step "Committing squashed changes..."
$commitMessage = "Merge changes from remote repository`n`nSquashed merge of all changes from $RemoteRepoUrl"
$authorString = 'coasting-nc <' + $AuthorEmail + '>'
git commit -m $commitMessage --author=$authorString
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to commit changes"
    Write-Warning-Custom "You may need to commit manually"
    exit 1
}
Write-Success "Changes committed"

Write-Step "Removing remote '$RemoteAlias'..."
git remote remove $RemoteAlias
if ($LASTEXITCODE -ne 0) {
    Write-Warning-Custom "Failed to remove remote"
} else {
    Write-Success "Remote removed"
}

Write-Step "Deleting temporary branch '$tempBranch'..."
git branch -d $tempBranch
if ($LASTEXITCODE -ne 0) {
    Write-Warning-Custom "Failed to delete temporary branch"
} else {
    Write-Success "Temporary branch deleted"
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  MERGE COMPLETED SUCCESSFULLY!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Review the changes: git log -1 --stat" -ForegroundColor White
Write-Host "2. Verify the commit author: git log -1" -ForegroundColor White
Write-Host "3. Check for any issues: git status" -ForegroundColor White
Write-Host "4. If everything looks good, push:" -ForegroundColor White
Write-Host "   git push origin main" -ForegroundColor Yellow
Write-Host ""
Write-Host "To undo this merge (before pushing):" -ForegroundColor Cyan
Write-Host '   git reset --hard HEAD~1' -ForegroundColor Red
Write-Host ""
