
# set product names, allows mapping of (devel) name in Project to a more consistent name in release
$settings = @{}

# internal project name in project
$settings.sourceName = $args[1]    # i.e. "Templatemodul"
# public project name i.e. in github  
$settings.targetName = $args[2]    # i.e. "TemplateModule" 

if ($args.length -gt 3) {
    $settings.useCreator = $args[3]
} else {
    $settings.compileWith = "openknxproducer"
}

# the following properties might be derived, but also defined
if (!$settings.targetName) {
    # if we leave target name empty, source an target name are identical
    $settings.targetName = $settings.sourceName
}
$settings.releaseIndication = $args[0]
if ($settings.releaseIndication) {
    # name of the release (Release, Beta, Big, ...)
    $settings.appRelease = $settings.releaseIndication
    # main xml file name will be accessed with the following name (default is i.e. <sourceName>-Release.xml)
    $settings.releaseName = "$($settings.sourceName)-$($settings.releaseIndication)"
} else {
    # name of default release if you do not specify any specific release
    $settings.appRelease = "Beta"
    # main xml file name will be accessed with the following name (default is i.e. <sourceName>.xml)
    $settings.releaseName = "$($settings.sourceName)"
}

Return $settings