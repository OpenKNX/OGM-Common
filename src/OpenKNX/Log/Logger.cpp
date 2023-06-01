#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Facade.h"


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
        }

        void Logger::mutex_block()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_enter_blocking(&_mutex);
#endif
        }

        void Logger::mutex_unblock()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_exit(&_mutex);
#endif
        }

        void Logger::color(uint8_t color)
        {
            STATE_BY_CORE(_color) = color;
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

        void Logger::log(const std::string prefix, const std::string message, ...)
        {
            va_list args;
            va_start(args, message);
            log(prefix, message, args);
            va_end(args);
        }

        void Logger::log(const std::string message)
        {
            mutex_block();
            clearPreviouseLine();
            if (isColorSet())
                printColorCode();
            printCore();
            printMessage(message);
            if (isColorSet())
                printColorCode(0);
            SERIAL_DEBUG.println();
            printPrompt();
            mutex_unblock();
        }

        void Logger::log(const std::string prefix, const std::string message, va_list args)
        {
            mutex_block();
            clearPreviouseLine();
            if (isColorSet())
                printColorCode();
            printCore();
            printPrefix(prefix);
            printIndent();
            printMessage(message, args);
            if (isColorSet())
                printColorCode(0);
            SERIAL_DEBUG.println();
            printPrompt();
            mutex_unblock();
        }

        void Logger::logHex(const std::string prefix, const uint8_t* data, size_t size)
        {
            mutex_block();
            clearPreviouseLine();
            printCore();
            printPrefix(prefix);
            printIndent();
            printHex(data, size);
            SERIAL_DEBUG.println();
            printPrompt();
            mutex_unblock();
        }

        bool Logger::isColorSet()
        {
            return STATE_BY_CORE(_color) != 0;
        }

        void Logger::printColorCode(uint8_t color)
        {
            if (color > 0)
            {
                SERIAL_DEBUG.print(ANSI_CODE "[38;5;");
                SERIAL_DEBUG.print((int)color);
                SERIAL_DEBUG.print("m");
            }
            else
            {
                SERIAL_DEBUG.print(ANSI_RESET);
            }
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
                    SERIAL_DEBUG.print("0");

                SERIAL_DEBUG.print(data[i], HEX);
                SERIAL_DEBUG.print(" ");
            }
        }

        void Logger::clearPreviouseLine()
        {
            while (_lastConsoleLen > 0)
            {
                _lastConsoleLen--;
                SERIAL_DEBUG.print("\b");
            }
            SERIAL_DEBUG.print("\33[K");
        }

        void Logger::printPrompt()
        {
            clearPreviouseLine();
            SERIAL_DEBUG.print(openknx.console.prompt);
            _lastConsoleLen = strlen(openknx.console.prompt);
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

        void Logger::printCore()
        {
#if defined(ARDUINO_ARCH_RP2040) && (defined(OPENKNX_DEBUG) || defined(OPENKNX_LOGGER_SHOWCORE))
            if (openknx.common.usesDualCore())
                SERIAL_DEBUG.print(rp2040.cpuid() ? "_1> " : "0_> ");
#endif
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

#if defined(OPENKNX_TRACE1) || defined(OPENKNX_TRACE2) || defined(OPENKNX_TRACE3) || defined(OPENKNX_TRACE4) || defined(OPENKNX_TRACE5)
        bool Logger::checkTrace(std::string prefix)
        {
            MatchState ms;
            ms.Target((char*)prefix.c_str());
#ifdef OPENKNX_TRACE1
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE1)) > 0 && ms.MatchCount (TRACE_STRINGIFY(OPENKNX_TRACE1))>0)
                return true;
#endif
#ifdef OPENKNX_TRACE2
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE2)) > 0 && ms.MatchCount (TRACE_STRINGIFY(OPENKNX_TRACE2))>0)
                return true;
#endif
#ifdef OPENKNX_TRACE3
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE3)) > 0 && ms.MatchCount (TRACE_STRINGIFY(OPENKNX_TRACE3))>0)
                return true;
#endif
#ifdef OPENKNX_TRACE4
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE4)) > 0 && ms.MatchCount (TRACE_STRINGIFY(OPENKNX_TRACE4))>0)
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
            {
                SERIAL_DEBUG.print("  ");
            }
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
    } // namespace Log
} // namespace OpenKNX