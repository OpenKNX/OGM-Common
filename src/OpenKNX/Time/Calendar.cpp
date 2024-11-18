#include "OpenKNX.h"
#ifdef LOG_HolidayKo
    #include "Logic.h"
#endif

namespace OpenKNX
{
    namespace Time
    {
        bool Calendar::isValid()
        {
            return openknx.time.isValid();
        }

        DateOnly Calendar::getLocalDate()
        {
            return openknx.time.getLocalTime();
        }

        DateOnly Calendar::getUtcDate()
        {
            return openknx.time.getUtcTime();
        }

        DateOnly Calendar::getEaster()
        {
            DateTime localTime = openknx.time.getLocalTime();
            if (_easter.year != localTime.year)
            {
                _easter = localTime;
                // calculate easter
                uint16_t lYear = localTime.year;
                uint8_t a = lYear % 19;
                uint8_t b = lYear % 4;
                uint8_t c = lYear % 7;

                uint8_t k = lYear / 100;
                uint8_t q = k / 4;
                uint8_t p = ((8 * k) + 13) / 25;
                uint8_t Egz = (38 - (k - q) + p) % 30; // Die Jahrhundertepakte
                uint8_t M = (53 - Egz) % 30;
                uint8_t N = (4 + k - q) % 7;

                uint8_t d = ((19 * a) + M) % 30;
                uint8_t e = ((2 * b) + (4 * c) + (6 * d) + N) % 7;

                // calculate easter:
                if ((22 + d + e) <= 31)
                {
                    _easter.day = 22 + d + e;
                    _easter.month = 3 - 1;
                }
                else
                {
                    _easter.day = d + e - 9;
                    _easter.month = 4 - 1;

                    // handle two exceptions
                    if (_easter.day == 26)
                        _easter.day = 19;
                    else if ((_easter.day == 25) && (d == 28) && (a > 10))
                        _easter.day = 18;
                }
            }
            return _easter;
        }

        DateOnly Calendar::getForthAdvent()
        {
            DateTime localTime = openknx.time.getLocalTime();
            if (_fourthAdvent.year != localTime.year)
            {
                tm fourthAdvent = {0};
                fourthAdvent.tm_year = localTime.year - 1900;
                fourthAdvent.tm_mon = 11;
                fourthAdvent.tm_mday = 24;
                fourthAdvent.tm_hour = 12;
                fourthAdvent.tm_min = 0;
                fourthAdvent.tm_sec = 0;
                fourthAdvent.tm_isdst = 0;
                mktime(&fourthAdvent);
                _fourthAdvent.year = localTime.year;
                _fourthAdvent.month = 12;
                _fourthAdvent.day = 24 - fourthAdvent.tm_wday;
            }
            return _fourthAdvent;
        }

#ifdef LOG_HolidayKo
        // Functions are currently depending on the logic modul implementation of holiday calculation
        bool Calendar::isHolidayToday()
        {
            return Timer::instance().holidayToday();
        }

        bool Calendar::isHolidayTommorow()
        {
            return Timer::instance().holidayTomorrow();
        }

        bool Calendar::isWorkingDayToday()
        {
            int wday = openknx.time.getLocalTime().dayOfWeek;
            return wday > 0 && wday < 6 && // Monday to Friday
                   !isHolidayToday();
        }

        bool Calendar::isWorkingDayTommorow()
        {
            int wday = openknx.time.getLocalTime().dayOfWeek;
            return wday >= 0 && wday < 5 && // Sunday to Thuersday
                   !isHolidayTommorow();
        }
#endif

    } // namespace Time
} // namespace OpenKNX