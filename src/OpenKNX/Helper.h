#pragma once
#include "knx.h"
#include <Arduino.h>
#include <cstdint>
#include <stdarg.h>
#include <stdio.h>

#define MAX_LOG_PREFIX 23

namespace OpenKNX
{
    class Helper
    {
      public:
        static void log(const char* output);
        static void log(const char* prefix, const char* output, ...);
        static void logHex(const char* prefix, const uint8_t* data, size_t size);
#ifdef ARDUINO_ARCH_RP2040
        static void nukeFlashKnxOnly();
        static void nukeFlash();
        static void nukeFlash(uint32_t offset, size_t bytes);
#endif
    };
} // namespace OpenKNX
