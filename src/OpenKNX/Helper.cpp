#include "OpenKNX/Helper.h"
#include "OpenKNX/Common.h"

#ifdef ARDUINO_ARCH_RP2040
#include "hardware/flash.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#endif

namespace OpenKNX
{
    void Helper::log(const char* output)
    {
        SERIAL_DEBUG.println(output);
    }

    void Helper::log(const char* prefix, const char* output, ...)
    {
        char buffer[200];
        va_list args;
        va_start(args, output);
        vsnprintf(buffer, 256, output, args);
        va_end(args);

        SERIAL_DEBUG.print(prefix);
        SERIAL_DEBUG.print(": ");
        size_t prefixLen = strlen(prefix);
        for (size_t i = 0; i < (MAX_LOG_PREFIX - prefixLen); i++)
        {
            SERIAL_DEBUG.print(" ");
        }
        SERIAL_DEBUG.println(buffer);
    }

    void Helper::logHex(const char* prefix, const uint8_t* data, size_t size)
    {
        SERIAL_DEBUG.print(prefix);
        SERIAL_DEBUG.print(": ");
        size_t prefixLen = strlen(prefix);
        for (size_t i = 0; i < (MAX_LOG_PREFIX - prefixLen); i++)
        {
            SERIAL_DEBUG.print(" ");
        }
        for (size_t i = 0; i < size; i++)
        {
            if (data[i] < 0x10)
            {
                SERIAL_DEBUG.print("0");
            }
            SERIAL_DEBUG.print(data[i], HEX);
            SERIAL_DEBUG.print(" ");
        }
        SERIAL_DEBUG.println();
    }

#ifdef ARDUINO_ARCH_RP2040
    /*
     * Erase whole flash from knx stack (pa, parameters, ... flash storage)
     * would probably be better off in the knx stack
     */
    void Helper::nukeFlashKnxOnly()
    {
        nukeFlash(KNX_FLASH_OFFSET, KNX_FLASH_SIZE);
    }

    /*
     * Erase whole flash also firmware!
     */
    void Helper::nukeFlash()
    {
        uint flash_size_bytes;
#ifndef PICO_FLASH_SIZE_BYTES
#warning PICO_FLASH_SIZE_BYTES not set, assuming 16M
        flash_size_bytes = 16 * 1024 * 1024;
#else
        flash_size_bytes = PICO_FLASH_SIZE_BYTES;
#endif
        nukeFlash(0, flash_size_bytes);
    }

    void Helper::nukeFlash(uint32_t offset, size_t bytes)
    {  
#ifdef WATCHDOG
        Watchdog.enable(2147483647);
#endif
        log("Nuker", "nuke flash ( %i -> %i)", offset, bytes);
        delay(10);
        flash_range_erase(offset, bytes);
        delay(10);
        watchdog_reboot(0,0,0);
    }
#endif

} // namespace OpenKNX
