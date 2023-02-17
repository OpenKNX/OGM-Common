#include "OpenKNX/Helper.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Helper::log(const char* message)
    {
        openknx.logger.log(LogLevel::Info, message);
    }

    void Helper::log(LogLevel loglevel, const char* message)
    {
        openknx.logger.log(loglevel, message);
    }

    void Helper::log(const char* prefix, const char* message, ...)
    {
        va_list args;
        va_start(args, message);
        openknx.logger.log(LogLevel::Info, prefix, message, args);
        va_end(args);
    }

    void Helper::log(LogLevel loglevel, const char* prefix, const char* message, ...)
    {
        va_list args;
        va_start(args, message);
        openknx.logger.log(loglevel, prefix, message, args);
        va_end(args);
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
} // namespace OpenKNX
