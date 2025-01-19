#pragma once

#ifndef SMALL_GROUPOBJECT
#error OGM-Common needs build-flag "-D SMALL_GROUPOBJECT"
#endif

#if !defined(ARDUINO_ARCH_SAMD) && !defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_ESP32)
#error Your architecture is not supported by OpenKNX
#endif


#ifndef ARDUINO_ARCH_ESP32
    #ifndef OPENKNX_FLASH_OFFSET
        #error "OPENKNX_FLASH_OFFSET is not defined"
    #endif

    #ifndef OPENKNX_FLASH_SIZE
        #error "OPENKNX_FLASH_SIZE is not defined"
    #endif

    #ifndef KNX_FLASH_OFFSET
        #error "KNX_FLASH_OFFSET is not defined"
    #endif

    #ifndef KNX_FLASH_SIZE
        #error "KNX_FLASH_SIZE is not defined"
    #endif
#endif

#ifdef ARDUINO_ARCH_RP2040
    #if OPENKNX_FLASH_SIZE % 2
        #error "OPENKNX_FLASH_SIZE cannot be divided by 2"
    #endif

    #if OPENKNX_FLASH_SIZE % 4096
        #error "OPENKNX_FLASH_SIZE must be multiple of 4096"
    #endif
#endif

#include "macros.h"
#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/Hardware.h"
#include "OpenKNX/Helper.h"
#include "OpenKNX/Module.h"