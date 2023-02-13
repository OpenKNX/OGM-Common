#pragma once
#include "knx.h"
#include "pico/sync.h"

#ifndef OPENKNX_MAX_LOG_PREFIX_LENGTH
#define OPENKNX_MAX_LOG_PREFIX_LENGTH 23
#endif

#ifndef OPENKNX_MAX_LOG_MESSAGE_LENGTH
#define OPENKNX_MAX_LOG_MESSAGE_LENGTH 200
#endif

namespace OpenKNX
{
    enum LogLevel
    {
        Error,
        Info,
        Debug,
        Trace
    };

    class Logger
    {
      private:
        mutex_t _mutex;
        void mutex_block();
        void mutex_unblock();
        void printMessage(const char* message, va_list args);
        void printMessage(const char* message);
        void printPrefix(const char* prefix);

      public:
        Logger();
        void log(LogLevel level, const char* prefix, const char* message, va_list args);
        void log(LogLevel level, const char* prefix, const char* message, ...);
        void log(LogLevel level, const char* prefix, const char* message);
        void log(LogLevel level, const char* message);
    };
} // namespace OpenKNX