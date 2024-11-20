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
            if (openknx.time.isValid())
            {
                DateTime utc = openknx.time.getUtcTime();
                if (utc.hour != _lastHour || utc.minute != _lastMinute)
                {
                    _lastHour = utc.hour;
                    _lastMinute = utc.minute;
                    recalculateSunCalculation(utc);
                }
            }
        }

        void SunCalculation::recalculateSunCalculation(DateTime &utc)
        {
            double latitude = 0;//ParamBASE_Latitude;
            double longitude = 0;//ParamBASE_Longitude;
            cTime cTime = {0};
            cTime.iYear = utc.year;
            cTime.iMonth = utc.month;
            cTime.iDay = utc.day;
            cTime.dHours = utc.hour;
            cTime.dMinutes = utc.minute;
            cTime.dSeconds = 0;

            cLocation cLocation = {0};
            cLocation.dLatitude = latitude;
            cLocation.dLongitude = longitude;

            cSunCoordinates cSunCoordinates;
            sunpos(cTime, cLocation, &cSunCoordinates);
            _azimut = cSunCoordinates.dAzimuth;
            _elevation = 90 - cSunCoordinates.dZenithAngle;

            double rise, set;
            // sunrise/sunset calculation
            SunRiseAndSet::sunRiseSet(utc.year, utc.month, utc.day,
                                      longitude, latitude, -35.0 / 60.0, 1, &rise, &set);

            _sunRiseUtc.hour = (int)floor(rise);
            _sunRiseUtc.minute = (int)(60 * (rise - floor(rise)));
            _sunRiseUtc.second = 0;
            _sunRiseLocalTime = DateTime(utc.year, utc.month, utc.day, _sunRiseUtc.hour, _sunRiseUtc.minute, _sunRiseUtc.second, DateTimeTypeUTC).toLocalTime();

            _sunSetUtc.hour = (int)floor(set);
            _sunSetUtc.minute = (int)(60 * (set - floor(set)));
            _sunSetUtc.second = 0;
            _sunSetLocalTime = DateTime(utc.year, utc.month, utc.day, _sunSetUtc.hour, _sunSetUtc.minute, _sunSetUtc.second, DateTimeTypeUTC).toLocalTime();

            _sunCalculationValid = true;
        }

        bool SunCalculation::processCommand(std::string &cmd, bool diagnoseKo)
        {
            if (cmd == "sun")
            {
                if (isSunCalculatioValid())
                {
                    //logInfoP("Used cordinates: %lf %lf", (double)ParamBASE_Latitude, (double)ParamBASE_Longitude);
                    logInfoP("Elevation: %f, Azimut: %f", _elevation, _azimut);
                    logInfoP("Sun rise: %02d::%02d UTC", _sunRiseUtc.hour, _sunRiseUtc.minute);
                    logInfoP("Sun rise: %02d::%02d (%s)", _sunRiseLocalTime.hour, _sunRiseLocalTime.minute, _sunRiseLocalTime.isDst ? "DST" : "ST");
                    logInfoP("Sun set: %02d::%02d UTC", _sunSetUtc.hour, _sunSetUtc.minute);
                    logInfoP("Sun set: %02d::%02d (%s)", _sunSetLocalTime.hour, _sunSetLocalTime.minute, _sunSetLocalTime.isDst ? "DST" : "ST");
                }
                else
                    logInfoP("Sun position not valid because valid time is missing");
                return true;
            }
            return false;
        }

    } // namespace Sun
} // namespace OpenKNX