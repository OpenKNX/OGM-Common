#pragma once

#include "Arduino.h"
namespace OpenKNX
{
    namespace Time
    { 
#ifndef ARDUINO_ARCH_SAMD
        class TimeClockSystem
        {
          public:
           void setup() {};
            void setTime(time_t epoch, unsigned long millisReceivedTimestamp) 
            {
                auto now = millis();
                auto millisOffset = now - millisReceivedTimestamp;
                auto seconds = (long)millisOffset / 1000;
                auto milliseconds = (long)millisOffset % 1000;
                struct timeval tv;
                tv.tv_sec = epoch + seconds;
                tv.tv_usec = milliseconds * 1000;
                timezone tz{0};
                settimeofday(&tv, &tz);
            }
            time_t getTime()
            {
                time_t now;
                return time(&now);
            }
        };
#endif
    } // namespace Time
} // namespace OpenKNX