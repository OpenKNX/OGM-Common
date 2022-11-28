$subprojects = Get-ChildItem -Directory lib
Write-Output "------- Build with -------" > dependencies.txt
foreach ($subproject in $subprojects) {
    git --git-dir lib/$subproject/.git log -1 --pretty=format:"%h $subproject" >> dependencies.txt
}
