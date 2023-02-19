#include "OpenKNX/Helper.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Helper::log(const std::string message)
    {
        openknx.logger.log(message);
    }

    void Helper::log(const std::string prefix, const std::string message, ...)
    {
        va_list args;
        va_start(args, message);
        openknx.logger.log(prefix, message, args);
        va_end(args);
    }

    void Helper::logHex(const std::string prefix, const uint8_t* data, size_t size)
    {
        openknx.logger.printPrefix(prefix);
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
