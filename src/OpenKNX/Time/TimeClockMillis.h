#pragma once
#include "Arduino.h"
namespace OpenKNX
{
    namespace Time
    {
        class TimeClockMillis
        {
            unsigned long _timeSetMs = 0;
            time_t _offset = 0;

          public:
            void setup() {};
            void loop() {};
            void setTime(time_t epoch, unsigned long millisReceivedTimestamp)
            {
                _offset = epoch;
                _timeSetMs = millisReceivedTimestamp;
            }
            time_t getTime()
            {
                return _offset + (millis() - _timeSetMs) / 1000;
            }
        };
    } // namespace Time
} // namespace OpenKNX