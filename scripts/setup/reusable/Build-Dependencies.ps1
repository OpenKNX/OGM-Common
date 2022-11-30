Write-Output "------- Build with -------" > dependencies.txt
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
    git --git-dir $subproject/.git log -1 --pretty=format:"%h $branch $subproject" >> dependencies.txt
}



