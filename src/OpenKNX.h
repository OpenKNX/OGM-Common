#pragma once

#ifndef SMALL_GROUPOBJECT
#error OGM-Common needs build-flag "-D SMALL_GROUPOBJECT"
#endif

#if !defined(ARDUINO_ARCH_SAMD) && !defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_ESP32)
#error Your archetecture is not supported by OpenKNX
#endif

#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/Hardware.h"
#include "OpenKNX/Helper.h"
#include "OpenKNX/Module.h"