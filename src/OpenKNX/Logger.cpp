#include "OpenKNX/Logger.h"
#include "OpenKNX/Common.h"

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
            mutex_enter_blocking(&_mutex);
#endif
    }

    void Logger::mutex_unblock()
    {
#ifdef ARDUINO_ARCH_RP2040
        if (openknx.usesDualCore())
            mutex_exit(&_mutex);
#endif
    }

    void Logger::color(uint8_t color)
    {
        _color = color;
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
        if (_color > 0)
            printColorCode(_color);
        printMessage(message);
        if (_color > 0)
            printColorCode(0);
        SERIAL_DEBUG.println();
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
        if (_color > 0)
            printColorCode(_color);
        printPrefix(prefix);
        printIndent();
        printMessage(message, args);
        if (_color > 0)
            printColorCode(0);
        SERIAL_DEBUG.println();
        mutex_unblock();
    }

    void Logger::logHex(const std::string prefix, const uint8_t* data, size_t size)
    {
        mutex_block();
        printPrefix(prefix);
        printIndent();
        printHex(data, size);
        SERIAL_DEBUG.println();
        mutex_unblock();
    }

    void Logger::printColorCode(uint8_t color)
    {
        if (color > 0)
        {
            SERIAL_DEBUG.printf(ANSI_COLOR("%i"), color);
        }
        else
        {
            SERIAL_DEBUG.print(ANSI_RESET);
        }
    }

    void Logger::printHex(const uint8_t* data, size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            if (data[i] < 0x10)
                SERIAL_DEBUG.print("0");

            SERIAL_DEBUG.print(data[i], HEX);
            SERIAL_DEBUG.print(" ");
        }
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
        SERIAL_DEBUG.print(message.c_str());
    }
    void Logger::printMessage(const std::string message, va_list args)
    {
        char buffer[OPENKNX_MAX_LOG_MESSAGE_LENGTH];
        vsnprintf(buffer, OPENKNX_MAX_LOG_MESSAGE_LENGTH, message.c_str(), args);
        SERIAL_DEBUG.print(buffer);
    }

    bool Logger::checkTrace(std::string prefix)
    {
#ifdef TRACE_LOG1
        if (TRACE_STRINGIFY(TRACE_LOG1) != "" && prefix.rfind(TRACE_STRINGIFY(TRACE_LOG1), 0) == 0)
            return true;
#endif
#ifdef TRACE_LOG2
        if (TRACE_STRINGIFY(TRACE_LOG2) != "" && prefix.rfind(TRACE_STRINGIFY(TRACE_LOG2), 0) == 0)
            return true;
#endif
#ifdef TRACE_LOG3
        if (TRACE_STRINGIFY(TRACE_LOG3) != "" && prefix.rfind(TRACE_STRINGIFY(TRACE_LOG3), 0) == 0)
            return true;
#endif
#ifdef TRACE_LOG4
        if (TRACE_STRINGIFY(TRACE_LOG4) != "" && prefix.rfind(TRACE_STRINGIFY(TRACE_LOG4), 0) == 0)
            return true;
#endif
#ifdef TRACE_LOG5
        if (TRACE_STRINGIFY(TRACE_LOG5) != "" && prefix.rfind(TRACE_STRINGIFY(TRACE_LOG5), 0) == 0)
            return true;
#endif

        return false;
    }

    void Logger::printIndent()
    {
        for (size_t i = 0; i < _indent; i++)
        {
            SERIAL_DEBUG.print("  ");
        }
    }

    void Logger::indentUp()
    {
        if (_indent == 10)
        {
            logError("Logger", "Indent error!");
        }
        else
        {
            _indent++;
        }
    }
    void Logger::indentDown()
    {
        if (_indent == 0)
        {
            logError("Logger", "Indent error!");
        }
        else
        {
            _indent--;
        }
    }
} // namespace OpenKNX