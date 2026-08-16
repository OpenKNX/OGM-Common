#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Facade.h"

#ifdef OPENKNX_RTT
    #include "SEGGER_RTT.h"
#endif


namespace OpenKNX
{
    namespace Log
    {
        Logger::Logger()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_init(&_mutex);
#elif defined(ARDUINO_ARCH_ESP32)
            _mutex = xSemaphoreCreateRecursiveMutex();
#endif
        }

        void Logger::init()
        {
#ifdef OPENKNX_LOGGER_DEVICE
            OPENKNX_LOGGER_DEVICE.begin(115200);
#endif
        }

#ifdef OPENKNX_WEBCONSOLE
        void Logger::appendWebconsoleBuffer(const char* s)
        {
            if (!s) return;
            while (*s)
                _ringBuf[_ringWritePos++ % RING_SIZE] = *s++;
        }

        void Logger::appendWebconsoleBuffer(int n)
        {
            char tmp[12];
            snprintf(tmp, sizeof(tmp), "%d", n);
            appendWebconsoleBuffer(tmp);
        }
#endif // OPENKNX_WEBCONSOLE

        void Logger::begin()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_enter_blocking(&_mutex);
#elif defined(ARDUINO_ARCH_ESP32)
            xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
#endif
        }

        void Logger::end()
        {
#ifdef ARDUINO_ARCH_RP2040
            recursive_mutex_exit(&_mutex);
#elif defined(ARDUINO_ARCH_ESP32)
            xSemaphoreGiveRecursive(_mutex);
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
#ifdef OPENKNX_WEBCONSOLE
            appendWebconsoleBuffer("\n");
#endif
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
                logWithPrefixAndValues(prefix, message, values);
            else
                logWithPrefix(prefix, message);
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

        void Logger::logHeader(const char* header)
        {
            beforeLog();
            printMessage("========================");
            auto len = strlen(header);
            if (len > 0)
            {
                printMessage(" ");
                printMessage(header);
                printMessage(" ");
                len += 2;
            }
            for (int i = 55 - len; i >= 0; i--)
                printMessage("=");
            afterLog();
        }

        void Logger::logDividingLine()
        {
            log("--------------------------------------------------------------------------------");
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
#ifdef OPENKNX_WEBCONSOLE
            appendWebconsoleBuffer("\x1B[");
            appendWebconsoleBuffer((int)color);
            appendWebconsoleBuffer("m");
#endif
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
                {
                    OPENKNX_LOGGER_DEVICE.print("0");
#ifdef OPENKNX_WEBCONSOLE
                    appendWebconsoleBuffer("0");
#endif
                }
                OPENKNX_LOGGER_DEVICE.print(data[i], HEX);
                OPENKNX_LOGGER_DEVICE.print(" ");
#ifdef OPENKNX_WEBCONSOLE
                char hex[4];
                snprintf(hex, sizeof(hex), "%X ", data[i]);
                appendWebconsoleBuffer(hex);
#endif
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
#ifdef OPENKNX_WEBCONSOLE
                    char c[2] = {prefix[i], '\0'};
                    appendWebconsoleBuffer(c);
#endif
                }
                else if (i == prefixLen && prefixLen > 0)
                {
                    OPENKNX_LOGGER_DEVICE.print(":");
#ifdef OPENKNX_WEBCONSOLE
                    appendWebconsoleBuffer(":");
#endif
                }
                else
                {
                    OPENKNX_LOGGER_DEVICE.print(" ");
#ifdef OPENKNX_WEBCONSOLE
                    appendWebconsoleBuffer(" ");
#endif
                }
            }
        }

        void Logger::printCore()
        {
#if defined(OPENKNX_DUALCORE) && (defined(OPENKNX_DEBUG) || defined(OPENKNX_LOGGER_SHOWCORE))
            if (openknx.usesDualCore())
            {
    #if defined(ARDUINO_ARCH_RP2040)
                const char* coreStr = rp2040.cpuid() ? "_1> " : "0_> ";
                OPENKNX_LOGGER_DEVICE.print(coreStr);
    #elif defined(ARDUINO_ARCH_ESP32)
                const char* coreStr = xPortGetCoreID() ? "_1> " : "0_> ";
                OPENKNX_LOGGER_DEVICE.print(coreStr);
    #endif
    #ifdef OPENKNX_WEBCONSOLE
                appendWebconsoleBuffer(coreStr);
    #endif
            }
#endif
        }

        void Logger::printMessage(const char* message)
        {
            OPENKNX_LOGGER_DEVICE.print(message);
#ifdef OPENKNX_WEBCONSOLE
            appendWebconsoleBuffer(message);
#endif
        }

        void Logger::printMessage(const char* message, va_list& values)
        {
            const char* found = strchr(message, '%');
            if (found == NULL)
            {
                OPENKNX_LOGGER_DEVICE.print(message);
#ifdef OPENKNX_WEBCONSOLE
                appendWebconsoleBuffer(message);
#endif
                return;
            }

            memset(_buffer.output, 0, OPENKNX_MAX_LOG_MESSAGE_LENGTH);
            uint16_t len = vsnprintf(_buffer.output, OPENKNX_MAX_LOG_MESSAGE_LENGTH, message, values);
            OPENKNX_LOGGER_DEVICE.print(_buffer.output);
#ifdef OPENKNX_WEBCONSOLE
            appendWebconsoleBuffer(_buffer.output);
#endif
            if (len >= OPENKNX_MAX_LOG_MESSAGE_LENGTH)
            {
                for (uint8_t i = 0; i < 4; i++)
                    if (_buffer.wall[i] != _buffer.magic[i])
                        openknx.hardware.fatalError(FATAL_SYSTEM, "BufferOverflow: increase OPENKNX_MAX_LOG_MESSAGE_LENGTH");
#ifdef OPENKNX_DEBUG
                printColorCode(33);
                OPENKNX_LOGGER_DEVICE.print("<-- Potential buffer overflow, please shorten your message");
                printColorCode(0);
#endif
            }
        }

#if defined(OPENKNX_TRACE)
        bool Logger::matchPart(const char* filter, size_t filterLen, const char* target, size_t targetLen)
        {
            // trailing '*' -> startsWith
            if (filterLen > 0 && filter[filterLen - 1] == '*')
            {
                size_t prefixLen = filterLen - 1;
                return targetLen >= prefixLen && strncmp(filter, target, prefixLen) == 0;
            }
            // exact match
            return filterLen == targetLen && strncmp(filter, target, filterLen) == 0;
        }

        bool Logger::matchTraceFilterList(const char* list, const char* target)
        {
            // split list at ';'; commas are reserved for sub lists (e.g. "Channel<4,5,7>")
            const char* start = list;
            for (const char* p = list;; ++p)
            {
                if (*p == '\0' || *p == ';')
                {
                    size_t len = (size_t)(p - start);
                    if (len > 0 && matchTraceFilter(start, len, target))
                        return true;
                    if (*p == '\0')
                        break;
                    start = p + 1;
                }
            }
            return false;
        }

        bool Logger::matchTraceFilter(const char* filter, size_t filterLen, const char* target)
        {
            const char* filterEnd = filter + filterLen;

            // split filter into prefix and optional sub (between '<' and '>')
            const char* fSub = (const char*)memchr(filter, '<', filterLen);
            size_t fPrefixLen = fSub ? (size_t)(fSub - filter) : filterLen;

            // split target into prefix and optional sub
            const char* tSub = strchr(target, '<');
            size_t tPrefixLen = tSub ? (size_t)(tSub - target) : strlen(target);

            // compare prefix
            if (!matchPart(filter, fPrefixLen, target, tPrefixLen))
                return false;

            // no sub part in filter -> ignore target's sub
            if (!fSub)
                return true;

            // filter has a sub part but target has none -> no match
            if (!tSub)
                return false;

            // determine sub contents (strip surrounding '<' and trailing '>')
            const char* fSubStart = fSub + 1;
            const char* fSubEnd = (const char*)memchr(fSubStart, '>', (size_t)(filterEnd - fSubStart));
            size_t fSubLen = fSubEnd ? (size_t)(fSubEnd - fSubStart) : (size_t)(filterEnd - fSubStart);

            const char* tSubStart = tSub + 1;
            const char* tSubEnd = strchr(tSubStart, '>');
            size_t tSubLen = tSubEnd ? (size_t)(tSubEnd - tSubStart) : strlen(tSubStart);

            // iterate over comma separated list elements of the filter sub
            const char* elem = fSubStart;
            const char* fSubLimit = fSubStart + fSubLen;
            while (elem < fSubLimit)
            {
                const char* comma = (const char*)memchr(elem, ',', (size_t)(fSubLimit - elem));
                size_t elemLen = comma ? (size_t)(comma - elem) : (size_t)(fSubLimit - elem);

                // numeric range "lo-hi" (only when no wildcard and a '-' separates two numbers)
                const char* dash = (const char*)memchr(elem, '-', elemLen);
                if (dash && dash != elem && (elemLen == 0 || elem[elemLen - 1] != '*'))
                {
                    char* endLo = nullptr;
                    char* endHi = nullptr;
                    long lo = strtol(elem, &endLo, 10);
                    long hi = strtol(dash + 1, &endHi, 10);
                    // valid range only if both sides fully numeric
                    if (endLo == dash && endHi == elem + elemLen)
                    {
                        char* endTarget = nullptr;
                        long val = strtol(tSubStart, &endTarget, 10);
                        if (endTarget == tSubStart + tSubLen && val >= lo && val <= hi)
                            return true;
                        elem = comma ? comma + 1 : fSubLimit;
                        continue;
                    }
                }

                if (matchPart(elem, elemLen, tSubStart, tSubLen))
                    return true;

                elem = comma ? comma + 1 : fSubLimit;
            }

            return false;
        }

        bool Logger::checkTrace(const std::string& prefix)
        {
            const char* target = prefix.c_str();
            if (strlen(TRACE_STRINGIFY(OPENKNX_TRACE)) > 0 && matchTraceFilterList(TRACE_STRINGIFY(OPENKNX_TRACE), target))
                return true;
            return false;
        }
#endif

        void Logger::printIndent()
        {
            for (size_t i = 0; i < getIndent(); i++)
            {
                OPENKNX_LOGGER_DEVICE.print("  ");
#ifdef OPENKNX_WEBCONSOLE
                appendWebconsoleBuffer("  ");
#endif
            }
        }

        void Logger::indentUp()
        {
            if (getIndent() == 10)
                logError("Logger", "Indent error!");
            else
                STATE_BY_CORE(_indent) += 1;
        }

        void Logger::indentDown()
        {
            if (getIndent() == 0)
                logError("Logger", "Indent error!");
            else
                STATE_BY_CORE(_indent) -= 1;
        }

        void Logger::indent(uint8_t indent)
        {
            STATE_BY_CORE(_indent) = indent;
        }

        uint8_t Logger::getIndent()
        {
            return STATE_BY_CORE(_indent);
        }

        void Logger::logOpenKnxHeader()
        {
            // OLD - No output
        }

        void Logger::printTimestamp()
        {
#ifndef ARDUINO_ARCH_SAMD
            if (openknx.time.isValid())
            {
                char tsBuf[24];
                auto t = openknx.time.getUtcTime();
                snprintf(tsBuf, sizeof(tsBuf), "%04i-%02i-%02i %02i:%02i:%02i", t.year, t.month, t.day, t.hour, t.minute, t.second);
                OPENKNX_LOGGER_DEVICE.print(tsBuf);
    #ifdef OPENKNX_WEBCONSOLE
                appendWebconsoleBuffer(tsBuf);
    #endif
            }
            else
#endif
            {
                std::string up = buildUptime();
                OPENKNX_LOGGER_DEVICE.print(up.c_str());
#ifdef OPENKNX_WEBCONSOLE
                appendWebconsoleBuffer(up.c_str());
#endif
            }
            OPENKNX_LOGGER_DEVICE.print(": ");
#ifdef OPENKNX_WEBCONSOLE
            appendWebconsoleBuffer(": ");
#endif
        }

        std::string Logger::buildUptime()
        {
            return humanDuration(uptime());
        }
    } // namespace Log
} // namespace OpenKNX
