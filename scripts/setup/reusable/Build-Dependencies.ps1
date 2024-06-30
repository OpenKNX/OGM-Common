# Initial Write-Output to dependencies.txt
# This is a OpenKNX dependencies file. You can enter information about your project dependencies here.
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
$initContent = @"
------- Built with -------
"@

Write-Output $initContent > dependencies.txt
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
            $branch = "?" # use invalid branch-name as fallback (prevents restore to branch)
            $info = "-> no branch '" + $subproject + "'"
            Write-Host -ForegroundColor DarkYellow $info
        }
        $info1 = git --git-dir $subproject/.git log -1 --pretty=format:"%h $branch $subproject"
        $info2 = git --git-dir $subproject/.git config --get remote.origin.url
        $info = $info1 + " " + $info2
        Write-Output $info >> dependencies.txt
    } else {
        $info = "-> ignore directory '" + $subproject + "'"
        Write-Host -ForegroundColor Red $info
    }
}



