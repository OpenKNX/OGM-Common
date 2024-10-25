#include "SunCalculation.h"
#include "OpenKNX.h"
#include "SunPos.h"
#include "SunRiseAndSet.h"

namespace OpenKNX
{
    namespace Sun
    {
        const std::string SunCalculation::logPrefix()
        {
            return "Sun";
        }
       
        void SunCalculation::loop()
        {
            if (openknx.time.isTimeValid())
            {
                auto utc = openknx.time.getUtcTime();
                if (utc.tm_hour != _lastHour || utc.tm_min != _lasMinute)
                {
                    _lastHour = utc.tm_hour;
                    _lasMinute = utc.tm_min;
                    recalculateSunCalculation(utc);
                }
            }
        }

        void SunCalculation::recalculateSunCalculation(tm& utc)
        {
            double latitude = ParamBASE_Latitude;
            double longitude = ParamBASE_Longitude;

            cTime cTime = {0};
            cTime.iYear = utc.tm_year + 1900;
            cTime.iMonth = utc.tm_mon + 1;
            cTime.iDay = utc.tm_mday;
            cTime.dHours = utc.tm_hour;
            cTime.dMinutes = utc.tm_min;
            cTime.dSeconds = 0;

            logDebugP("%04d-%02d-%02d %02d:%02d", cTime.iYear, cTime.iMonth, cTime.iDay, (int) cTime.dHours, (int) cTime.dMinutes);

            cLocation cLocation = {0};
            cLocation.dLatitude = latitude;
            cLocation.dLongitude = longitude;

            cSunCoordinates cSunCoordinates;
            sunpos(cTime, cLocation, &cSunCoordinates);
            _azimut = cSunCoordinates.dAzimuth;
            _elevation = 90 - cSunCoordinates.dZenithAngle;

            double rise, set;
            // sunrise/sunset calculation
            SunRiseAndSet::sunRiseSet(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                                      longitude, latitude, -35.0 / 60.0, 1, &rise, &set);

            _sunRiseUtc.tm_year = utc.tm_year;
            _sunRiseUtc.tm_mon = utc.tm_mon;
            _sunRiseUtc.tm_mday = utc.tm_mday;
            _sunRiseUtc.tm_hour = (int)floor(rise);
            _sunRiseUtc.tm_min = (int)(60 * (rise - floor(rise)));
            _sunRiseUtc.tm_sec = 0;
            _sunRiseLocalTime = openknx.time.convertUtcToLocalTime(_sunRiseUtc);

            _sunSetUtc.tm_year = utc.tm_year;
            _sunSetUtc.tm_mon = utc.tm_mon;
            _sunSetUtc.tm_mday = utc.tm_mday;
            _sunSetUtc.tm_hour = (int)floor(set);
            _sunSetUtc.tm_min = (int)(60 * (set - floor(set)));
            _sunSetUtc.tm_sec = 0;
            _sunSetLocalTime = openknx.time.convertUtcToLocalTime(_sunSetUtc);

            _sunCalculationValid = true;
        }

      

        bool SunCalculation::processCommand(std::string &cmd, bool diagnoseKo)
        {
            if (cmd == "sun")
            {
                if (isSunCalculatioValid())
                {
                    logInfoP("Elevation: %f, Azimut: %f", _elevation, _azimut);
                    logInfoP("Used cordinates: %lf %lf", (double)ParamBASE_Latitude, (double)ParamBASE_Longitude);
                    logInfoP("Sun rise: %02d::%02d UTC", _sunRiseUtc.tm_hour, _sunRiseUtc.tm_min);
                    logInfoP("Sun rise: %02d::%02d (%s)", _sunRiseLocalTime.tm_hour, _sunRiseLocalTime.tm_min, _sunRiseLocalTime.tm_isdst ? "Summertime" : "Wintertime");
                    logInfoP("Sun set: %02d::%02d UTC", _sunSetUtc.tm_hour, _sunSetUtc.tm_min);
                    logInfoP("Sun set: %02d::%02d (%s)", _sunSetLocalTime.tm_hour, _sunSetLocalTime.tm_min, _sunSetLocalTime.tm_isdst ? "Summertime" : "Wintertime");
                }
                else
                    logInfoP("Sun position now valid because valid time is missing");
                return true;
            }
            return false;
        }

    } // namespace Sun
} // namespace OpenKNX