#pragma once
#include "knx.h"
#include <string>
#ifdef ARDUINO_ARCH_RP2040
#include "pico/sync.h"
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
#if defined(TRACE_LOG1) || defined(TRACE_LOG2) || defined(TRACE_LOG3) || defined(TRACE_LOG4) || defined(TRACE_LOG5)
#include <regex>

#ifndef TRACE_LOG1
#define TRACE_LOG1
#endif
#ifndef TRACE_LOG2
#define TRACE_LOG2
#endif
#ifndef TRACE_LOG3
#define TRACE_LOG3
#endif
#ifndef TRACE_LOG4
#define TRACE_LOG4
#endif
#ifndef TRACE_LOG5
#define TRACE_LOG5
#endif

#define TRACE_STRINGIFY2(X) #X
#define TRACE_STRINGIFY(X) TRACE_STRINGIFY2(X)
// Force Debug Mode during Trace
#undef DEBUG_LOG
#define DEBUG_LOG
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

#ifdef DEBUG_LOG
#define logDebug(...)         \
    openknx.logger.color(8);  \
    openknx.logger.log(__VA_ARGS__); \
    openknx.logger.color(0)
#define logDebugP(...)                     \
    openknx.logger.color(8);               \
    openknx.logger.log(logPrefix(), __VA_ARGS__); \
    openknx.logger.color(0)
#define logHexDebug(...)         \
    openknx.logger.color(8);     \
    openknx.logger.logHex(__VA_ARGS__); \
    openknx.logger.color(0)
#define logHexDebugP(...)                     \
    openknx.logger.color(8);                  \
    openknx.logger.logHex(logPrefix(), __VA_ARGS__); \
    openknx.logger.color(0)
#else
#define logDebug(...)
#define logDebugP(...)
#define logHexDebug(...)
#define logHexDebugP(...)
#endif

namespace OpenKNX
{
    class Logger
    {
      private:
        volatile uint8_t _color = 0;
        volatile uint8_t _indent = 0;
#ifdef ARDUINO_ARCH_RP2040
        mutex_t _mutex;
#endif
        void mutex_block();
        void mutex_unblock();
        void printHex(const uint8_t* data, size_t size);
        void printMessage(const std::string message, va_list args);
        void printMessage(const std::string message);
        void printPrefix(const std::string prefix);
        void printColorCode(uint8_t color);
        void printIndent();

      public:
        Logger();

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

        bool checkTrace(const std::string prefix);
    };
} // namespace OpenKNX
