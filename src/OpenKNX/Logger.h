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

// #if > 3
// #define logInfo() openknx.....
// #else
// #define logInfo()
// #endif

// #define logError(args1 a2 a3)
// #define logError1(message)
// #define logError2(prefix, message, ...)
// #define logInfo(prefix, message, ...)
// #define logDebug(prefix, message, ...)
// #define logTrace(traceName, prefix, message, ...)


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
    };
} // namespace OpenKNX