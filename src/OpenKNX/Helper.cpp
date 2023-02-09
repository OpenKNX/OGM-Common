#include "OpenKNX/Helper.h"
#include "OpenKNX/Common.h"

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

    /*
     * Erase whole flash from knx stack (pa, parameters, ... flash storage)
     * would probably be better off in the knx stack
     */
    void Helper::nukeKnxFlash()
    {
        uint8_t* flashStart = knx.platform().getNonVolatileMemoryStart();
        size_t flashSize = knx.platform().getNonVolatileMemorySize();

        // TODO
#ifdef WATCHDOG
        openknx.watchdogLoop();
#endif
        log("Nuker", "nuke knx flash");
        knx.platform().restart();
    }

    /*
     * Erase whole flash also firmware!
     */
    void Helper::nukeWholeFlash()
    {
        // TODO
#ifdef WATCHDOG
        openknx.watchdogLoop();
#endif
        log("Nuker", "nuke whole flash");
        knx.platform().restart();
    }

} // namespace OpenKNX
