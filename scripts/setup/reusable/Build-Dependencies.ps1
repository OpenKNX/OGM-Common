# Create a File "dependencies.txt" to document the _exact_ dependencies of an OpenKNX OAM project.

# == Content of "dependencies.txt" ==
# Each line represents a dependency and should contain the following information:
# Hash value, branch, folder path, Git URL, and optionally the branch name.
#
# Example:
# a1b2c3d4 master lib/MyProject https://github.com/username/myproject.git#master
# a2b3c4d5 dev_1 lib/OpenKNX https://github.com/username/OpenKNX.git
#
# You can also add comments by starting a line with '#'.
# Example:
# # This is a comment.
#
# Please note that lines starting with '-------' are ignored.
$dependencies = @()
$dependencies += "------- Built with -------"


$failed = $false

$subprojects = Get-ChildItem -Directory lib
# $project = $(Split-Path $(Get-Location) -Leaf)
$projects = @()
# $projects += "." #  $project
foreach ($subproject in $subprojects) {
    $projects += "lib/" + $subproject.Name
}

# $branch = git branch --show-current
# $subproject = Split-Path $(Get-Location) -Leaf 
# git log -1 --pretty=format:"%h $branch $subproject" >> dependencies.txt
foreach ($subproject in $projects) {
    $branch = git --git-dir $subproject/.git branch --show-current
    if ($?) { # if the lib is no git repo, skip it
        if (!$branch) {
            Write-Host "ERROR: No branch selected for '$subproject' (detached HEAD)" -ForegroundColor Red
            $failed = $true
            # TODO check; set fallback as long not ending here
            $branch = "?????"
        }

        # check of all not inclueded in git
        $status = git --git-dir $subproject/.git --work-tree=$subproject status --porcelain

        # check for uncommited changes
        $changes = $status | Where-Object { $_ -notmatch '^\?\?' -and $_ }
        if ($changes) {
            Write-Host "WARN: '$subproject' contains uncommited changes! Please check following git status ..." -ForegroundColor Magenta
            $changes
            Write-Host ""
        }

        # check for untracked files
        $untracked = $status | Where-Object { $_ -match '^\?\?' }
        if ($untracked) {
            Write-Host "WARN: '$subproject' contains untracked files! Please check following git status ..." -ForegroundColor Magenta
            $untracked
            Write-Host ""
        }

        $commitHash = git --git-dir $subproject/.git log -1 --pretty=format:"%h"
        $remoteUrl = git --git-dir $subproject/.git config --get remote.origin.url
        $dependencies += "$commitHash $branch $subproject $remoteUrl"
    } else {
        $info = "-> ignore directory '" + $subproject + "'"
        Write-Output $info
    }
}

if ($failed) {
    Write-Host "ABORT: No update of 'dependencies.txt', ERRORs exist!" -ForegroundColor Red
    exit 1
}

$dependencies | Set-Content dependencies.txt -Encoding UTF8