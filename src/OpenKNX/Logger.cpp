#include "OpenKNX/Logger.h"
#include "OpenKNX/Common.h"

// void logError()
// {
//     openknx.log("afsdsdfs", "sdfsdfsdds");
// }

namespace OpenKNX
{
    Logger::Logger()
    {
#ifdef ARDUINO_ARCH_RP2040
        mutex_init(&_mutex);
#endif
    }
    void Logger::mutex_block()
    {
#ifdef ARDUINO_ARCH_RP2040
        if (openknx.usesDualCore())
        {
            mutex_enter_blocking(&_mutex);
        }
#endif
    }

    void Logger::mutex_unblock()
    {
#ifdef ARDUINO_ARCH_RP2040
        if (openknx.usesDualCore())
        {
            mutex_exit(&_mutex);
        }
#endif
    }

    std::string Logger::logPrefix(const std::string prefix, const std::string id)
    {
        char buffer[OPENKNX_MAX_LOG_PREFIX_LENGTH];
        sprintf(buffer, "%s<%s>", prefix.c_str(), id.c_str());
        return std::string(buffer);
    }

    std::string Logger::logPrefix(const std::string prefix, const int id)
    {
        char buffer[OPENKNX_MAX_LOG_PREFIX_LENGTH];
        sprintf(buffer, "%s<%i>", prefix.c_str(), id);
        return std::string(buffer);
    }

    void Logger::log(const std::string message)
    {
        mutex_block();
        printMessage(message);
        mutex_unblock();
    }

    void Logger::log(const std::string prefix, const std::string message, ...)
    {
        va_list args;
        va_start(args, message);
        log(prefix, message, args);
        va_end(args);
    }

    void Logger::log(const std::string prefix, const std::string message, va_list args)
    {
        mutex_block();
        printPrefix(prefix);
        printMessage(message, args);
        mutex_unblock();
    }

    void Logger::printPrefix(const std::string prefix)
    {
        size_t prefixLen = MIN(strlen(prefix.c_str()), OPENKNX_MAX_LOG_PREFIX_LENGTH);
        for (size_t i = 0; i < (OPENKNX_MAX_LOG_PREFIX_LENGTH + 2); i++)
        {
            if (i < prefixLen)
            {
                SERIAL_DEBUG.print(prefix.c_str()[i]);
            }
            else if (i == prefixLen && prefixLen > 0)
            {
                SERIAL_DEBUG.print(":");
            }
            else
            {
                SERIAL_DEBUG.print(" ");
            }
        }
    }

    void Logger::printMessage(const std::string message)
    {
        SERIAL_DEBUG.println(message.c_str());
    }
    void Logger::printMessage(const std::string message, va_list args)
    {
        char buffer[OPENKNX_MAX_LOG_MESSAGE_LENGTH];
        vsnprintf(buffer, OPENKNX_MAX_LOG_MESSAGE_LENGTH, message.c_str(), args);
        SERIAL_DEBUG.println(buffer);
    }
} // namespace OpenKNX