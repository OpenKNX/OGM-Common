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
            void loop() {};
            void setTime(time_t epoch, unsigned long millisReceivedTimestamp)
            {
                unsigned long now = millis();
                unsigned long millisOffset = now - millisReceivedTimestamp;
                long seconds = (long)millisOffset / 1000;
                long milliseconds = (long)millisOffset % 1000;
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