
# set product names, allows mapping of (devel) name in Project to a more consistent name in release
$settings = lib/OGM-Common/scripts/build/OpenKNX-Build-Settings.ps1 $args[0] "Templatemodul" "Templatemodule"

# $settings.sourceName="Templatemodul"  
# $settings.targetName="Templatemodule" 
# $settings.knxprod="src/{0}.h" -f $settings.sourceName
# $settings.hardware="src/{0}Hardware.h" -f $settings.sourceName

Return $settings