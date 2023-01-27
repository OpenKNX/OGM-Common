#include "OpenKNX/Helper.h"

namespace OpenKNX
{
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

    void Helper::deactivatePowerTrail() {
        
    }

    void Helper::activatePowerTrail() {
        
    }


} // namespace OpenKNX
