#pragma once
#include "TimeClock.h"
#include <sys/time.h>
namespace OpenKNX
{
    namespace Time
    {
#ifndef ARDUINO_ARCH_SAMD
        class TimeClockSystem : public TimeClock
        {
          public:
            inline void setup() override {}
            inline void loop() override {}

            inline void setTime(time_t epoch, unsigned long millisReceivedTimestamp) override
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

            inline time_t getTime() override
            {
                time_t now;
                return time(&now);
            }
            inline bool isRunning() override { return true; }
        };
#endif
    } // namespace Time
} // namespace OpenKNX