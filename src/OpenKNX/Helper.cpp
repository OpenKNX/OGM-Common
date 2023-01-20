#include "OpenKNX/Helper.h"

namespace OpenKNX
{
    uint32_t Helper::paramTimer(uint8_t base, uint16_t time)
    {
        return 0;
    }

    void Helper::debug(const char* prefix, const char* data, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, data);
        int result = vsnprintf(buffer, 256, data, args);
        va_end(args);
        SERIAL_DEBUG.print(prefix);
        SERIAL_DEBUG.print(": ");
        SERIAL_DEBUG.println(buffer);
    }

    void Helper::debugHex(const char* prefix, const uint8_t* data, size_t size)
    {
        SERIAL_DEBUG.print(prefix);
        SERIAL_DEBUG.print(": ");
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

} // namespace OpenKNX
