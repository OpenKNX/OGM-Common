#pragma once
#include "knx.h"
#ifdef ARDUINO_ARCH_RP2040
#include "pico/sync.h"
#endif

#ifndef OPENKNX_MAX_LOG_PREFIX_LENGTH
#define OPENKNX_MAX_LOG_PREFIX_LENGTH 23
#endif

#ifndef OPENKNX_MAX_LOG_MESSAGE_LENGTH
#define OPENKNX_MAX_LOG_MESSAGE_LENGTH 200
#endif

namespace OpenKNX
{
    enum class LogLevel
    {
        Error,
        Info,
        Debug,
        Trace
    };

    class Logger
    {
      private:
#ifdef ARDUINO_ARCH_RP2040
        mutex_t _mutex;
#endif
        void mutex_block();
        void mutex_unblock();
        void printMessage(const char* message, va_list args);
        void printMessage(const char* message);
        void printPrefix(const char* prefix);

      public:
        Logger();
        void log(LogLevel level, const char* prefix, const char* message, va_list args);
        void log(LogLevel level, const char* prefix, const char* message, ...);
        void log(LogLevel level, const char* message);
    };
} // namespace OpenKNX