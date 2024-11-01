#pragma once
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
            uint8_t _lastHour = 0;
            uint8_t _lasMinute = 0;
            bool _sunCalculationValid = false;
#ifdef OPENKNX_SUN_POSITION
            float _azimut = 0;
            float _elevation = 0;
#endif
            const std::string logPrefix();
            void loop();
            bool processCommand(std::string& cmd, bool diagnoseKo);
            void recalculateSunCalculation(tm& utc);
            tm _sunRiseUtc;
            tm _sunSetUtc;
            tm _sunRiseLocalTime;
            tm _sunSetLocalTime;

          public:
            /*
             * Returns true, if the sun position is calculated which requires a valid time
             */
            bool isSunCalculatioValid() { return _sunCalculationValid; }
#ifdef OPENKNX_SUN_POSITION
            /*
             * Returns the azimut
             */
            float azimut() { return _azimut; }
            /*
             * Returns the elevation
             */
            float elevation() { return _elevation; }
#endif
            /*
             * Returns the sun rise time in UTC for the current day
             */
            tm sunRiseUtc() { return _sunRiseUtc; }

            /*
             * Returns the sun set time in UTC for the current day
             */
            tm sunSetUtc() { return _sunSetUtc; }

            /*
             * Returns the sun rise time in local time for the current day
             */
            tm sunRiseLocalTime() { return _sunRiseLocalTime; }

            /*
             * Returns the sun set time in local time for the current day
             */
            tm sunSetLocalTime() { return _sunSetLocalTime; }
        };
    } // namespace Sun
} // namespace OpenKNX