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
                const DateTime localTime = openknx.time.getLocalTime();
                const bool dayChanged = localTime.day != _lastDay || localTime.month != _lastMonth || localTime.year != _lastYear;
                if (dayChanged)
                {
                    _lastYear = localTime.year;
                    _lastMonth = localTime.month;
                    _lastDay = localTime.day;

                    recalculateSunRiseSet(localTime);
                }
                const bool minuteChanged = dayChanged || localTime.hour != _lastHour || localTime.minute != _lastMinute || localTime.isDst != _lastDst;
                if (minuteChanged)
                {
                    _lastHour = localTime.hour;
                    _lastMinute = localTime.minute;
                    _lastDst = localTime.isDst;

                    recalculateSunPos(localTime.toUtc());

                    _sunCalculationValid = true; // both were calculcated as dayChanged => minuteChanged
                }
            }
        }

        void SunCalculation::recalculateSunPos(const DateTime& utc)
        {
            const double latitude = ParamBASE_Latitude;
            const double longitude = ParamBASE_Longitude;

            cTime cTime = {0};
            cTime.iYear = utc.year;
            cTime.iMonth = utc.month;
            cTime.iDay = utc.day;
            cTime.dHours = utc.hour;
            cTime.dMinutes = utc.minute;
            cTime.dSeconds = 0; // ignore seconds, as typically calculated every minute only!

            cLocation cLocation = {0};
            cLocation.dLatitude = latitude;
            cLocation.dLongitude = longitude;

            cSunCoordinates cSunCoordinates;
            sunpos(cTime, cLocation, &cSunCoordinates);
            _azimuth = cSunCoordinates.dAzimuth;
            _elevation = 90 - cSunCoordinates.dZenithAngle;
        }

        void SunCalculation::recalculateSunRiseSet(const DateTime& localTime)
        {
            const double latitude = ParamBASE_Latitude;
            const double longitude = ParamBASE_Longitude;

            double rise, set;
            // sunrise/sunset calculation
            // TODO check the return {<,=,>}0 for special cases
            SunRiseAndSet::sunRiseSet(localTime.year, localTime.month, localTime.day,
                                      longitude, latitude, -35.0 / 60.0, 1, &rise, &set);

            const int32_t sunRiseUtcHour = (int32_t)floor(rise);
            const uint8_t sunRiseUtcMinute = (int32_t)(60 * (rise - floor(rise)));
            const uint8_t sunRiseUtcSecond = 0;
            DateTime dtRise = DateTime(localTime.year, localTime.month, localTime.day, sunRiseUtcHour, sunRiseUtcMinute, sunRiseUtcSecond, DateTimeTypeUTC);
            _sunRiseUtc = dtRise;
            _sunRiseLocalTime = dtRise.toLocalTime();

            const int32_t sunSetUtcHour = (int32_t)floor(set);
            const uint8_t sunSetUtcMinute = (int32_t)(60 * (set - floor(set)));
            const uint8_t sunSetUtcSecond = 0;
            DateTime dtSet = DateTime(localTime.year, localTime.month, localTime.day, sunSetUtcHour, sunSetUtcMinute, sunSetUtcSecond, DateTimeTypeUTC);
            _sunSetUtc = dtSet;
            _sunSetLocalTime = dtSet.toLocalTime();
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
