## General idea

As long as the file structure follows some concepts, the scripts in this directory can be used for a release build.

All files in *reusable* directory are project independent and can be used directly by the release build script. There should be no changes necessary.

All files in *templates* directory must be copied to the project *scripts* directory and have to be adjusted accordingly.

The root for a setup build is *templates/Build-Release.ps1*. See the comments within this file how to adjust it.

You can look in *OAM-LogicModule/scripts* for a complete working example.