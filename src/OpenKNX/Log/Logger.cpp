#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Facade.h"

#ifdef OPENKNX_RTT
    #include "SEGGER_RTT.h"
#endif

#if defined(OPENKNX_TRACE1) || defined(OPENKNX_TRACE2) || defined(OPENKNX_TRACE3) || defined(OPENKNX_TRACE4) || defined(OPENKNX_TRACE5)
    #include <Regexp.h>
#endif


namespace OpenKNX
{
    namespace Log
    {
        Logger::Logger()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_init(&_mutex);
#endif

#ifdef OPENKNX_LOGGER_DEVICE
            OPENKNX_LOGGER_DEVICE.begin(115200);
#endif
        }

        void Logger::begin()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_enter_blocking(&_mutex);
#endif
        }

        void Logger::end()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_exit(&_mutex);
#endif
        }

        void Logger::color(uint8_t color)
        {
            STATE_BY_CORE(_color) = color;
        }

        std::string Logger::buildPrefix(const std::string& prefix, const std::string& id)
        {
            return buildPrefix(prefix.c_str(), id.c_str());
        }

        std::string Logger::buildPrefix(const char* prefix, const char* id)
        {
            char buffer[OPENKNX_MAX_LOG_PREFIX_LENGTH] = {};
            sprintf(buffer, "%s<%s>", prefix, id);
            return std::string(buffer);
        }

        std::string Logger::buildPrefix(const std::string& prefix, const int id)
        {
            return buildPrefix(prefix.c_str(), id);
        }

        std::string Logger::buildPrefix(const char* prefix, const int id)
        {
            char buffer[OPENKNX_MAX_LOG_PREFIX_LENGTH] = {};
            sprintf(buffer, "%s<%i>", prefix, id);
            return std::string(buffer);
        }

        void Logger::beforeLog()
        {
            begin();
            clearPreviouseLine();
            if (isColorSet())
                printColorCode();
            printTimestamp();
            printCore();
        }

        void Logger::afterLog()
        {
            if (isColorSet())
                printColorCode(0);
            OPENKNX_LOGGER_DEVICE.println();
            printPrompt();
            end();
        }

        void Logger::log(const std::string& message)
        {
            log(message.c_str());
        }

        void Logger::log(const char* message)
        {
            beforeLog();
            printMessage(message);
            afterLog();
        }

        void Logger::logWithPrefix(const std::string& prefix, const std::string& message)
        {
            logWithPrefix(prefix.c_str(), message.c_str());
        }

        void Logger::logWithPrefix(const char* prefix, const char* message)
        {
            beforeLog();
            printPrefix(prefix);
            printIndent();
            printMessage(message);
            afterLog();
        }

        void Logger::logWithPrefixAndValues(const std::string& prefix, const std::string& message, ...)
        {
            va_list values;
            va_start(values, message);
            logWithPrefixAndValues(prefix.c_str(), message.c_str(), values);
            va_end(values);
        }

        void Logger::logWithPrefixAndValues(const char* prefix, const char* message, ...)
        {
            va_list values;
            va_start(values, message);
            logWithPrefixAndValues(prefix, message, values);
            va_end(values);
        }

        void Logger::logWithPrefixAndValues(const char* prefix, const char* message, va_list& values)
        {
            beforeLog();
            printPrefix(prefix);
            printIndent();
            printMessage(message, values);
            afterLog();
        }

        void Logger::logWithValues(const std::string& message, ...)
        {
            va_list values;
            va_start(values, message);
            logWithValues(message.c_str(), values);
            va_end(values);
        }

        void Logger::logWithValues(const char* message, ...)
        {
            va_list values;
            va_start(values, message);
            logWithValues(message, values);
            va_end(values);
        }

        void Logger::logWithValues(const char* message, va_list& values)
        {
            beforeLog();
            printMessage(message, values);
            afterLog();
        }

        void Logger::logHex(const uint8_t* data, size_t size)
        {
            beforeLog();
            printHex(data, size);
            afterLog();
        }

        void Logger::logHexWithPrefix(const std::string& prefix, const uint8_t* data, size_t size)
        {
            logHexWithPrefix(prefix.c_str(), data, size);
        }

        void Logger::logHexWithPrefix(const char* prefix, const uint8_t* data, size_t size)
        {
            beforeLog();
            printPrefix(prefix);
            printIndent();
            printHex(data, size);
            afterLog();
        }

        void Logger::logMacroWrapper(uint8_t logColor, const char* prefix, const char* message, ...)
        {
            va_list values;
            va_start(values, message);
            logMacroWrapper(logColor, prefix, message, values);
            va_end(values);
        }

        void Logger::logMacroWrapper(uint8_t logColor, const std::string& prefix, const char* message, ...)
        {
            va_list values;
            va_start(values, message);
            logMacroWrapper(logColor, prefix.c_str(), message, values);
            va_end(values);
        }

        void Logger::logMacroWrapper(uint8_t logColor, const std::string& prefix, const std::string& message, ...)
        {
            va_list values;
            va_start(values, message);
            logMacroWrapper(logColor, prefix.c_str(), message.c_str(), values);
            va_end(values);
        }

        void Logger::logMacroWrapper(uint8_t logColor, const char* prefix, const char* message, va_list& values)
        {
            color(logColor);
            const char* found = strchr(message, '%');
            if (found != NULL)
            {
                logWithPrefixAndValues(prefix, message, values);
            }
            else
            {
                logWithPrefix(prefix, message);
            }

            color(0);
        }

        void Logger::logHexMacroWrapper(uint8_t logColor, const std::string& prefix, const uint8_t* data, size_t size)
        {
            logHexMacroWrapper(logColor, prefix.c_str(), data, size);
        }

        void Logger::logHexMacroWrapper(uint8_t logColor, const char* prefix, const uint8_t* data, size_t size)
        {
            color(logColor);
            logHexWithPrefix(prefix, data, size);
            color(0);
        }

        bool Logger::isColorSet()
        {
            return STATE_BY_CORE(_color) != 0;
        }

        void Logger::printColorCode(uint8_t color)
        {
            OPENKNX_LOGGER_DEVICE.print("\x1B[");
            OPENKNX_LOGGER_DEVICE.print((int)color);
            OPENKNX_LOGGER_DEVICE.print("m");
        }

        void Logger::printColorCode()
        {
            printColorCode(STATE_BY_CORE(_color));
        }

        void Logger::printHex(const uint8_t* data, size_t size)
        {
            for (size_t i = 0; i < size; i++)
            {
                if (data[i] < 0x10)
                    OPENKNX_LOGGER_DEVICE.print("0");

                OPENKNX_LOGGER_DEVICE.print(data[i], HEX);
                OPENKNX_LOGGER_DEVICE.print(" ");
            }
        }

        void Logger::clearPreviouseLine()
        {
#ifndef OPENKNX_RTT
            OPENKNX_LOGGER_DEVICE.print("\33[2K\r");
#endif
        }

        void Logger::printPrompt()
        {
#ifndef OPENKNX_RTT
            clearPreviouseLine();
            OPENKNX_LOGGER_DEVICE.print("$ ");
            OPENKNX_LOGGER_DEVICE.print(openknx.console.prompt);
#endif
        }

        void Logger::printPrefix(const char* prefix)
        {
            size_t prefixLen = MIN(strlen(prefix), OPENKNX_MAX_LOG_PREFIX_LENGTH);
            for (size_t i = 0; i < (OPENKNX_MAX_LOG_PREFIX_LENGTH + 2); i++)
            {
                if (i < prefixLen)
                {
                    OPENKNX_LOGGER_DEVICE.print(prefix[i]);
                }
                else if (i == prefixLen && prefixLen > 0)
                {
                    OPENKNX_LOGGER_DEVICE.print(":");
                }
                else
                {
                    OPENKNX_LOGGER_DEVICE.print(" ");
                }
            }
        }

        void Logger::printCore()
        {
#if defined(OPENKNX_DUALCORE) && (defined(OPENKNX_DEBUG) || defined(OPENKNX_LOGGER_SHOWCORE))
            if (openknx.usesDualCore())
            {
    #if defined(ARDUINO_ARCH_RP2040)
                OPENKNX_LOGGER_DEVICE.print(rp2040.cpuid() ? "_1> " : "0_> ");
    #elif defined(ARDUINO_ARCH_ESP32)
                OPENKNX_LOGGER_DEVICE.print(xPortGetCoreID() ? "_1> " : "0_> ");
    #endif
            }
#endif
        }

        void Logger::printMessage(const char* message)
        {
            OPENKNX_LOGGER_DEVICE.print(message);
        }

        void Logger::printMessage(const char* message, va_list& values)
        {
            const char* found = strchr(message, '%');
            if (found == NULL)
            {
                OPENKNX_LOGGER_DEVICE.print(message);
                return;
            }

            memset(_buffer, 0, OPENKNX_MAX_LOG_MESSAGE_LENGTH);
            uint16_t len = vsnprintf(_buffer, OPENKNX_MAX_LOG_MESSAGE_LENGTH, message, values);
            OPENKNX_LOGGER_DEVICE.print(_buffer);
            if (len >= OPENKNX_MAX_LOG_MESSAGE_LENGTH)
                openknx.hardware.fatalError(FATAL_SYSTEM, "BufferOverflow: increase OPENKNX_MAX_LOG_MESSAGE_LENGTH");
        }

#if defined(OPENKNX_TRACE1) || defined(OPENKNX_TRACE2) || defined(OPENKNX_TRACE3) || defined(OPENKNX_TRACE4) || defined(OPENKNX_TRACE5)
        bool Logger::checkTrace(const std::string& prefix)
        {
            MatchState ms;
            ms.Target((char*)prefix.c_str());
    #ifdef OPENKNX_TRACE1
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE1)) > 0 && ms.MatchCount(TRACE_STRINGIFY(OPENKNX_TRACE1)) > 0)
                return true;
    #endif
    #ifdef OPENKNX_TRACE2
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE2)) > 0 && ms.MatchCount(TRACE_STRINGIFY(OPENKNX_TRACE2)) > 0)
                return true;
    #endif
    #ifdef OPENKNX_TRACE3
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE3)) > 0 && ms.MatchCount(TRACE_STRINGIFY(OPENKNX_TRACE3)) > 0)
                return true;
    #endif
    #ifdef OPENKNX_TRACE4
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE4)) > 0 && ms.MatchCount(TRACE_STRINGIFY(OPENKNX_TRACE4)) > 0)
                return true;
    #endif
    #ifdef OPENKNX_TRACE5
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE5)) > 0 && ms.MatchCount(TRACE_STRINGIFY(OPENKNX_TRACE5)))
                return true;
    #endif

            return false;
        }
#endif

        void Logger::printIndent()
        {
            for (size_t i = 0; i < getIndent(); i++)
                OPENKNX_LOGGER_DEVICE.print("  ");
        }

        void Logger::indentUp()
        {
            if (getIndent() == 10)
            {
                logError("Logger", "Indent error!");
            }
            else
            {
                STATE_BY_CORE(_indent)
                ++;
            }
        }

        void Logger::indentDown()
        {
            if (getIndent() == 0)
            {
                logError("Logger", "Indent error!");
            }
            else
            {
                STATE_BY_CORE(_indent)
                --;
            }
        }

        void Logger::indent(uint8_t indent)
        {
            STATE_BY_CORE(_indent) = indent;
        }

        uint8_t Logger::getIndent()
        {
            return STATE_BY_CORE(_indent);
        }

        // Open #
        // +----+
        // # KNX
        void Logger::logOpenKnxHeader()
        {
            // OLD - No output
        }

        void Logger::printTimestamp()
        {
            if (openknx.time.isValid())
                OPENKNX_LOGGER_DEVICE.printf("%04i-%02i-%02i %02i:%02i:%02i", openknx.time.getUtcTime().year, openknx.time.getUtcTime().month, openknx.time.getUtcTime().day, openknx.time.getUtcTime().hour, openknx.time.getUtcTime().minute, openknx.time.getUtcTime().second);
            else
                OPENKNX_LOGGER_DEVICE.print(buildUptime().c_str());
            OPENKNX_LOGGER_DEVICE.print(": ");
        }

        std::string Logger::buildUptime()
        {
            uint32_t secs = uptime();
            uint16_t days = secs / 86400;
            secs -= days * 86400;
            uint8_t hours = secs / 3600;
            secs -= hours * 3600;
            uint8_t mins = secs / 60;
            secs -= mins * 60;

            char result[26] = {};
            sprintf(result, "%dd %2.2d:%2.2d:%2.2d", (days % 10000), hours, mins, (uint8_t)secs);

            return result;
        }
    } // namespace Log
} // namespace OpenKNX