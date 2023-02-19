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

#define logError(...) openknx.logger.log(__VA_ARGS__)
#define logErrorP(...) openknx.logger.log(logPrefix(), __VA_ARGS__)
#define logInfo(...) openknx.logger.log(__VA_ARGS__)
#define logInfoP(...) openknx.logger.log(logPrefix(), __VA_ARGS__)

#if defined(TRACE_LOG1) || defined(TRACE_LOG2) || defined(TRACE_LOG3) || defined(TRACE_LOG4) || defined(TRACE_LOG5)

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
#define logTrace(prefix, ...)              \
    if (openknx.logger.checkTrace(prefix)) \
        openknx.logger.log(prefix, __VA_ARGS__);
#define logTraceP(...)                          \
    if (openknx.logger.checkTrace(logPrefix())) \
        openknx.logger.log(logPrefix(), __VA_ARGS__);
#else
#define logTrace(...)
#define logTraceP(...)
#endif

#ifdef DEBUG_LOG
#define logDebug(...) openknx.log(__VA_ARGS__)
#define logDebugP(...) openknx.log(logPrefix(), __VA_ARGS__)
#else
#define logDebug(...)
#define logDebugP(...)
#endif

namespace OpenKNX
{
    class Logger
    {
      private:
#ifdef ARDUINO_ARCH_RP2040
        mutex_t _mutex;
#endif
        void mutex_block();
        void mutex_unblock();
        void printMessage(const std::string message, va_list args);
        void printMessage(const std::string message);

      public:
        Logger();
        void printPrefix(const std::string prefix);

        std::string logPrefix(const std::string prefix, const std::string id);
        std::string logPrefix(const std::string prefix, const int id);

        void log(const std::string prefix, const std::string message, va_list args);
        void log(const std::string prefix, const std::string message, ...);
        void log(const std::string message);

        bool checkTrace(const std::string prefix);
    };
} // namespace OpenKNX
