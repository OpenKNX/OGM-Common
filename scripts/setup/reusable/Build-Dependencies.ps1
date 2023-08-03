Write-Output "------- Built with -------" > dependencies.txt
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
    if ($branch) { # if the lib is no git repo, skip it
        $info1 = git --git-dir $subproject/.git log -1 --pretty=format:"%h $branch $subproject"
        $info2 = git --git-dir $subproject/.git config --get remote.origin.url
        $info = $info1 + " " + $info2
        Write-Output $info >> dependencies.txt
    } else {
        $info = "-> ignore directory '" + $subproject + "'"
        Write-Output $info
    }
}



