#pragma once
/**
 * OpenKNX Hardware Definition Header File
 * 
 * Defines hardware properties for various reusable OpenKNX hardware modules and 
 * includes hardware definition headers for various OpenKNX modules.
 * 
 * Note: This file should only contain include directives for hardware definition headers.
 *       No hardware definitions should be made directly in this file.
 * 
 * Naming Conventions for OpenKNX Hardware Definitions:
 *      1. All hardware definition files should begin with "OpenKNX-".
 *      2. The name should describe the specific hardware type and Optional version.
 *      3. Use hyphens "-" to separate words.
 *      4. Example: "OpenKNX-PiPico-BCU-Hardware.h"
 * 
 * Naming Conventions for "OpenKNX Ready" 3rd Party Hardware Definitions:
 *      1. All hardware definition files should begin with "OpenKNXReady-".
 *      2. The name should describe the specific hardware type and Optional version.
 *      3. Use hyphens "-" to separate words.
 *      4. Example: "OpenKNXReady-XYZ-Hardware.h"
 */

/**
 * Include hardware definition headers:
 * 
 */


// OpenKNX Hardware Definitions
#include "HardwareConfig/OpenKNX/misc.h"
#include "HardwareConfig/OpenKNX/REG1.h"
#include "HardwareConfig/OpenKNX/REG2-PiPico.h"
#include "HardwareConfig/OpenKNX/UP1.h"
#include "HardwareConfig/OpenKNX/Xiao.h"

// OpenKNX Ready Hardware Definitions
//  #include "HardwareConfig/OpenKNX-Ready/AB-SmartHouse/xyz.h"
//  #include "HardwareConfig/OpenKNX-Ready/SMART-MF/xyz.h"

// Other 3rd Party Hardware Definitions
//  #include "HardwareConfig/Others/XYZ-Hardware.h"
