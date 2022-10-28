
# set product names, allows mapping of (devel) name in Project to a more consistent name in release
$settings = @{}

# internal project name in project
$settings.sourceName="Templatemodul"
# public project name i.e. in github  
$settings.targetName="TemplateModule" 

# the following properties might be derived, but also defined
$settings.knxprod="src/{0}.h" -f $settings.sourceName
# the name of the hardware definition header file 
$settings.hardware="src/{0}Hardware.h" -f $settings.sourceName

Return $settings