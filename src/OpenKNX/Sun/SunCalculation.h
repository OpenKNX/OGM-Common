#pragma once
#include "../DateTime.h"
#include "Arduino.h"
#include "chrono"
#include "string"
#include "time.h"

namespace OpenKNX
{
    class Common;
    class Console;

    namespace Sun
    {
        class SunCalculation
        {
            friend Common;
            friend Console;

            uint16_t _lastYear = 0;
            uint8_t _lastMonth = 0;
            uint8_t _lastDay = 0;
            uint8_t _lastHour = 0xff;
            uint8_t _lastMinute = 0xff;
            bool _lastDst = false; // will be updated/overwritten, as first check of Y,M,D will fail

            bool _sunCalculationValid = false;
            float _azimuth = 0;
            float _elevation = 0;

            const std::string logPrefix();
            void loop();
            bool processCommand(std::string& cmd, bool diagnoseKo);

            void recalculateSunPos(const DateTime& utc);
            void recalculateSunRiseSet(const DateTime& localTime);

            DateTime _sunRiseUtc;
            DateTime _sunSetUtc;
            DateTime _sunRiseLocalTime;
            DateTime _sunSetLocalTime;

          public:
            /*
             * Returns true, if the sun position is calculated which requires a valid time
             */
            bool isSunCalculatioValid() { return _sunCalculationValid; }
#ifdef OPENKNX_SUN_POSITION
            /*
             * Returns the azimuth
             */
            float azimuth() { return _azimuth; }
            /*
             * Returns the elevation
             */
            float elevation() { return _elevation; }
#endif
            /*
             * Returns the sun rise time in UTC for the current day
             */
            TimeOnly sunRiseUtc() { return _sunRiseUtc; }

            /*
             * Returns the sun set time in UTC for the current day
             */
            TimeOnly sunSetUtc() { return _sunSetUtc; }

            /*
             * Returns the sun rise time in local time for the current day
             */
            TimeOnly sunRiseLocalTime() { return _sunRiseLocalTime; }

            /*
             * Returns the sun set time in local time for the current day
             */
            TimeOnly sunSetLocalTime() { return _sunSetLocalTime; }
        };
    } // namespace Sun
} // namespace OpenKNX