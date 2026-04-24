#include "OpenKNX.h"

#ifdef ParamBASE_Latitude

#include "SunCalculation.h"
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
            double latitude = ParamBASE_Latitude;
            double longitude = ParamBASE_Longitude;
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
            _azimuth = cSunCoordinates.dAzimuth;
            _elevation = 90 - cSunCoordinates.dZenithAngle;

            double rise, set;
            // sunrise/sunset calculation
            // TODO check the return {<,=,>}0 for special cases
            SunRiseAndSet::sunRiseSet(utc.year, utc.month, utc.day,
                                      longitude, latitude, -35.0 / 60.0, 1, &rise, &set);

            const int32_t sunRiseUtcHour = (int32_t)floor(rise)
            const uint8_t sunRiseUtcMinute = (int32_t)(60 * (rise - floor(rise)));
            const uint8_t sunRiseUtcSecond = 0;
            DateTime dtRise = DateTime(utc.year, utc.month, utc.day, sunRiseUtcHour, sunRiseUtcMinute, sunRiseUtcSecond, DateTimeTypeUTC);
            _sunRiseUtc.hour = dtRise.hour;
            _sunRiseUtc.minute = dtRise.minute;
            _sunRiseUtc.second = dtRise.second;
            _sunRiseLocalTime = dtRise.toLocalTime();

            const int32_t sunSetUtcHour = (int32_t)floor(set);
            const uint8_t sunSetUtcMinute = (int32_t)(60 * (set - floor(set)));
            const uint8_t sunSetUtcSecond = 0;
            DateTime dtSet = DateTime(utc.year, utc.month, utc.day, sunSetUtcHour, sunSetUtcMinute, sunSetUtcSecond, DateTimeTypeUTC);
            _sunSetUtc.hour = dtSet.hour;
            _sunSetUtc.minute = dtSet.minute;
            _sunSetUtc.second = dtSet.second;
            _sunSetLocalTime = dtSet.toLocalTime();

            _sunCalculationValid = true;
        }

        bool SunCalculation::processCommand(std::string &cmd, bool diagnoseKo)
        {
            if (cmd == "sun" && !diagnoseKo)
            {
                if (isSunCalculatioValid())
                {
                    logInfoP("Used cordinates: %lf %lf", (double)ParamBASE_Latitude, (double)ParamBASE_Longitude);
                    logInfoP("Elevation: %f, Azimuth: %f", _elevation, _azimuth);
                    logInfoP("Sun rise: %02d::%02d UTC", _sunRiseUtc.hour, _sunRiseUtc.minute);
                    logInfoP("Sun rise: %02d::%02d (%s)", _sunRiseLocalTime.hour, _sunRiseLocalTime.minute, _sunRiseLocalTime.isDst ? "DST" : "ST");
                    logInfoP("Sun set: %02d::%02d UTC", _sunSetUtc.hour, _sunSetUtc.minute);
                    logInfoP("Sun set: %02d::%02d (%s)", _sunSetLocalTime.hour, _sunSetLocalTime.minute, _sunSetLocalTime.isDst ? "DST" : "ST");
                }
                else
                    logInfoP("Sun position not valid because valid time is missing");
                return true;
            } 
            else if (diagnoseKo && cmd.rfind("sun") == 0)
            {
                if (cmd.rfind("sun h") == 0)
                {
                    openknx.console.writeDiagnoseKo("-> help");
                    openknx.console.writeDiagnoseKo("");
                    openknx.console.writeDiagnoseKo("-> elevation");
                    openknx.console.writeDiagnoseKo("");
                    openknx.console.writeDiagnoseKo("-> azimuth");
                    openknx.console.writeDiagnoseKo("");
                    return true;
                }
                if (isSunCalculatioValid())
                {
                    if (cmd.rfind("sun e") == 0) 
                    {
                        openknx.console.writeDiagnoseKo("E: %2.5f", _elevation);
                        return true;
                    }
                    else if (cmd.rfind("sun a") == 0) 
                    {
                        openknx.console.writeDiagnoseKo("A: %3.5f", _azimuth);
                        return true;
                    } 
                }
            }
            return false;
        }

    } // namespace Sun
} // namespace OpenKNX

#endif