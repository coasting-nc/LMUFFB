#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Merges a remote repository into coasting-nc/LMUFFB as a single squashed commit.

.DESCRIPTION
    This script automates the process of merging changes from a remote repository
    into the current repository (assumed to be coasting-nc/LMUFFB) as a single
    squashed commit with coasting-nc as the author. All traces of the remote
    repository are removed after the merge.

.PARAMETER RemoteRepoUrl
    The URL of the remote repository to merge (e.g., https://github.com/user/repo)

.PARAMETER AccessToken
    Optional. GitHub personal access token (PAT) for accessing private repositories.
    The token will be used to authenticate the git fetch operation and will be
    removed along with the remote after the merge completes.

.PARAMETER RemoteAlias
    Optional. The alias to use for the remote repository. Default: "temp-remote"

.PARAMETER AuthorEmail
    Optional. The email to use for the commit author. Default: "coasting-nc@users.noreply.github.com"

.EXAMPLE
    .\merge_remote_repo.ps1 -RemoteRepoUrl "https://github.com/user/LMUFFB"

.EXAMPLE
    .\merge_remote_repo.ps1 -RemoteRepoUrl "https://github.com/user/LMUFFB" -AccessToken "ghp_yourTokenHere"

.EXAMPLE
    .\merge_remote_repo.ps1 -RemoteRepoUrl "https://github.com/user/repo" -AccessToken "ghp_token" -RemoteAlias "user-repo" -AuthorEmail "custom@email.com"

.NOTES
    - This script must be run from within the coasting-nc/LMUFFB repository
    - The script will NOT push changes automatically - manual review is required
    - If merge conflicts occur, the script will pause and you must resolve them manually
    - The access token is only stored temporarily and is removed when the script completes
#>

param(
    [Parameter(Mandatory=$true, HelpMessage="URL of the remote repository to merge")]
    [string]$RemoteRepoUrl,
    
    [Parameter(Mandatory=$false, HelpMessage="GitHub access token for private repositories")]
    [string]$AccessToken = "",
    
    [Parameter(Mandatory=$false)]
    [string]$RemoteAlias = "temp-remote",
    
    [Parameter(Mandatory=$false)]
    [string]$AuthorEmail = "coasting-nc@users.noreply.github.com"
)

# Set error action preference to stop on errors
$ErrorActionPreference = "Stop"

# Color output functions
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

# Verify we're in a git repository
Write-Step "Verifying git repository..."
try {
    $gitCheck = git rev-parse --is-inside-work-tree 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Not a git repository"
    }
    Write-Success "Git repository confirmed"
} catch {
    Write-Error-Custom "This script must be run from within a git repository"
    exit 1
}

# Check if we're on main branch
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

# Update main branch
Write-Step "Updating main branch from origin..."
git pull origin main
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to pull latest changes from origin/main"
    exit 1
}
Write-Success "Main branch updated"

# Check if remote already exists and remove it
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
    # Parse GitHub URL and insert token for authentication
    if ($RemoteRepoUrl -match '^https://github\.com/(.+)$') {
        $remoteUrlWithAuth = "https://$AccessToken@github.com/$($Matches[1])"
        Write-Success "Access token added"
    } else {
        Write-Warning-Custom "Could not parse GitHub URL, using URL as-is"
    }
}

# Add the remote repository
Write-Step "Adding remote repository as '$RemoteAlias'..."
git remote add $RemoteAlias $remoteUrlWithAuth
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to add remote repository"
    exit 1
}
Write-Success "Remote repository added"

# Fetch from the remote
Write-Step "Fetching from remote repository..."
git fetch $RemoteAlias
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to fetch from remote repository"
    git remote remove $RemoteAlias
    exit 1
}
Write-Success "Remote repository fetched"

# Create temporary branch
$tempBranch = "temp-merge-$(Get-Date -Format 'yyyyMMddHHmmss')"
Write-Step "Creating temporary branch '$tempBranch'..."
git checkout -b $tempBranch
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to create temporary branch"
    git remote remove $RemoteAlias
    exit 1
}
Write-Success "Temporary branch created"

# Merge remote/main into temporary branch
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

# Switch back to main
Write-Step "Switching back to main branch..."
git checkout main
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to checkout main branch"
    exit 1
}
Write-Success "Switched to main"

# Perform squash merge
Write-Step "Performing squash merge..."
git merge --squash $tempBranch
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Squash merge failed"
    git branch -D $tempBranch
    git remote remove $RemoteAlias
    exit 1
}
Write-Success "Squash merge completed"

# Commit the squashed changes
Write-Step "Committing squashed changes..."
$commitMessage = "Merge changes from remote repository`n`nSquashed merge of all changes from $RemoteRepoUrl"

# Build author string to avoid PowerShell parsing issues with angle brackets
$authorString = 'coasting-nc <' + $AuthorEmail + '>'
git commit -m $commitMessage --author=$authorString
if ($LASTEXITCODE -ne 0) {
    Write-Error-Custom "Failed to commit changes"
    Write-Warning-Custom "You may need to commit manually"
    exit 1
}
Write-Success "Changes committed"

# Remove the remote
Write-Step "Removing remote '$RemoteAlias'..."
git remote remove $RemoteAlias
if ($LASTEXITCODE -ne 0) {
    Write-Warning-Custom "Failed to remove remote (non-critical)"
} else {
    Write-Success "Remote removed"
}

# Delete temporary branch
Write-Step "Deleting temporary branch '$tempBranch'..."
git branch -d $tempBranch
if ($LASTEXITCODE -ne 0) {
    Write-Warning-Custom "Failed to delete temporary branch (non-critical)"
} else {
    Write-Success "Temporary branch deleted"
}

# Final summary
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  MERGE COMPLETED SUCCESSFULLY!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Review the changes: git log -1 --stat" -ForegroundColor White
Write-Host "2. Verify the commit author: git log -1" -ForegroundColor White
Write-Host "3. Check for any issues: git status" -ForegroundColor White
Write-Host "4. If everything looks good, push to remote:" -ForegroundColor White
Write-Host "   git push origin main" -ForegroundColor Yellow
Write-Host ""
Write-Host "To undo this merge (before pushing):" -ForegroundColor Cyan
Write-Host '   git reset --hard HEAD~1' -ForegroundColor Red
Write-Host ""
