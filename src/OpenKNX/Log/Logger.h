#pragma once
#include "Arduino.h"
#include <string>
#ifdef ARDUINO_ARCH_RP2040
#include "pico/sync.h"
#endif

#ifdef OPENKNX_RTT
#include <RTTStream.h>
#define OPENKNX_LOGGER_DEVICE openknx.logger.rtt
#else
#define OPENKNX_LOGGER_DEVICE SERIAL_DEBUG
#endif

#ifndef OPENKNX_MAX_LOG_PREFIX_LENGTH
#define OPENKNX_MAX_LOG_PREFIX_LENGTH 23
#endif

#ifndef OPENKNX_MAX_LOG_MESSAGE_LENGTH
#define OPENKNX_MAX_LOG_MESSAGE_LENGTH 200
#endif

#define ANSI_CODE "\033"
#define ANSI_RESET ANSI_CODE "[0m"
#define ANSI_COLOR(COLOR) ANSI_CODE "[38;5;" COLOR "m"

#define logIndentUp() openknx.logger.indentUp()
#define logIndentDown() openknx.logger.indentDown()
#define logIndent(X) openknx.logger.indent(X)

#define logError(...)                \
    openknx.logger.color(1);         \
    openknx.logger.log(__VA_ARGS__); \
    openknx.logger.color(0)
#define logErrorP(...)                            \
    openknx.logger.color(1);                      \
    openknx.logger.log(logPrefix(), __VA_ARGS__); \
    openknx.logger.color(0)
#define logHexError(...)                \
    openknx.logger.color(1);            \
    openknx.logger.logHex(__VA_ARGS__); \
    openknx.logger.color(0)
#define logHexErrorP(...)                            \
    openknx.logger.color(1);                         \
    openknx.logger.logHex(logPrefix(), __VA_ARGS__); \
    openknx.logger.color(0)
#define logInfo(...) openknx.logger.log(__VA_ARGS__)
#define logInfoP(...) openknx.logger.log(logPrefix(), __VA_ARGS__)
#define logHexInfo(...) openknx.logger.logHex(__VA_ARGS__)
#define logHexInfoP(...) openknx.logger.logHex(logPrefix(), __VA_ARGS__)
#if defined(OPENKNX_TRACE1) || defined(OPENKNX_TRACE2) || defined(OPENKNX_TRACE3) || defined(OPENKNX_TRACE4) || defined(OPENKNX_TRACE5)

#ifndef OPENKNX_TRACE1
#define OPENKNX_TRACE1
#endif
#ifndef OPENKNX_TRACE2
#define OPENKNX_TRACE2
#endif
#ifndef OPENKNX_TRACE3
#define OPENKNX_TRACE3
#endif
#ifndef OPENKNX_TRACE4
#define OPENKNX_TRACE4
#endif
#ifndef OPENKNX_TRACE5
#define OPENKNX_TRACE5
#endif

#define TRACE_STRINGIFY2(X) #X
#define TRACE_STRINGIFY(X) TRACE_STRINGIFY2(X)
// Force Debug Mode during Trace
#undef OPENKNX_DEBUG
#define OPENKNX_DEBUG
#define logTrace(prefix, ...)                    \
    if (openknx.logger.checkTrace(prefix))       \
    {                                            \
        openknx.logger.color(8);                 \
        openknx.logger.log(prefix, __VA_ARGS__); \
        openknx.logger.color(0);                 \
    }
#define logTraceP(...)                                \
    if (openknx.logger.checkTrace(logPrefix()))       \
    {                                                 \
        openknx.logger.color(8);                      \
        openknx.logger.log(logPrefix(), __VA_ARGS__); \
        openknx.logger.color(0);                      \
    }
#define logHexTrace(prefix, ...)                    \
    if (openknx.logger.checkTrace(prefix))          \
    {                                               \
        openknx.logger.color(8);                    \
        openknx.logger.logHex(prefix, __VA_ARGS__); \
        openknx.logger.color(0);                    \
    }
#define logHexTraceP(...)                                \
    if (openknx.logger.checkTrace(logPrefix()))          \
    {                                                    \
        openknx.logger.color(8);                         \
        openknx.logger.logHex(logPrefix(), __VA_ARGS__); \
        openknx.logger.color(0);                         \
    }
#else
#define logTrace(...)
#define logTraceP(...)
#define logHexTrace(...)
#define logHexTraceP(...)
#endif

#ifdef OPENKNX_DEBUG
#define logDebug(...)                \
    openknx.logger.color(8);         \
    openknx.logger.log(__VA_ARGS__); \
    openknx.logger.color(0)
#define logDebugP(...)                            \
    openknx.logger.color(8);                      \
    openknx.logger.log(logPrefix(), __VA_ARGS__); \
    openknx.logger.color(0)
#define logHexDebug(...)                \
    openknx.logger.color(8);            \
    openknx.logger.logHex(__VA_ARGS__); \
    openknx.logger.color(0)
#define logHexDebugP(...)                            \
    openknx.logger.color(8);                         \
    openknx.logger.logHex(logPrefix(), __VA_ARGS__); \
    openknx.logger.color(0)
#else
#define logDebug(...)
#define logDebugP(...)
#define logHexDebug(...)
#define logHexDebugP(...)
#endif

#ifdef ARDUINO_ARCH_RP2040
#define STATE_BY_CORE(X) X[rp2040.cpuid()]
#else
#define STATE_BY_CORE(X) X
#endif

/*
 * Fetches an exclusive lock to allow contiguous output.
 * This can be called multiple times per thread.
 *
 * Attention: The function blocks the other core if it also wants to output something.
 * The lock should be active as short as possible. Do not use it if you do not know what you are doing!
 */
#define logBegin() openknx.logger.begin();
/*
 * Release the exclusive lock.
 */
#define logEnd() openknx.logger.end();

namespace OpenKNX
{

    namespace Log
    {
        class Logger
        {
          private:
            uint8_t _lastConsoleLen = 0;
#ifdef ARDUINO_ARCH_RP2040
            // use individual values per core
            volatile uint8_t _color[2] = {(uint8_t)0, (uint8_t)0};
            volatile uint8_t _indent[2] = {(uint8_t)0, (uint8_t)0};
            recursive_mutex_t _mutex;
#else
            uint8_t _color = 0;
            uint8_t _indent = 0;
#endif

            void printHex(const uint8_t* data, size_t size);
            void printMessage(const std::string message, va_list args);
            void printMessage(const std::string message);
            void printPrefix(const std::string prefix);
            void printCore();
            bool isColorSet();
            void printColorCode(uint8_t color);
            void printColorCode();
            void printIndent();
            uint8_t getIndent();

          public:
#ifdef OPENKNX_RTT
            RTTStream rtt;
#endif
            Logger();

            /*
             * Fetches an exclusive lock to allow contiguous output.
             * This can be called multiple times per thread.
             *
             * Attention: The function blocks the other core if it also wants to output something.
             * The lock should be active as short as possible. Do not use it if you do not know what you are doing!
             */
            void begin();

            /*
             * Release the exclusive lock.
             */
            void end();

            std::string logPrefix(const std::string prefix, const std::string id);
            std::string logPrefix(const std::string prefix, const int id);

            void log(const std::string prefix, const std::string message, va_list args);
            void log(const std::string prefix, const std::string message, ...);
            void log(const std::string message);
            void logHex(const std::string prefix, const uint8_t* data, size_t size);
            void color(uint8_t color = 0);

            void indentUp();
            void indentDown();
            void indent(uint8_t indent);

#if defined(OPENKNX_TRACE1) || defined(OPENKNX_TRACE2) || defined(OPENKNX_TRACE3) || defined(OPENKNX_TRACE4) || defined(OPENKNX_TRACE5)
            bool checkTrace(const std::string prefix);
#endif
            void printPrompt();
            void clearPreviouseLine();
        };
    } // namespace Log
} // namespace OpenKNX
