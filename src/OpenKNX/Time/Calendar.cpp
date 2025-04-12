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

        DateOnly Calendar::calcEaster(uint16_t year)
        {
            const uint16_t lYear = year;

            // calculate easter
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

            DateOnly easter = {0};
            easter.year = year;

            // calculate easter:
            if ((22 + d + e) <= 31)
            {
                easter.day = 22 + d + e;
                easter.month = 3 - 1;
            }
            else
            {
                easter.day = d + e - 9;
                easter.month = 4 - 1;

                // handle two exceptions
                if (easter.day == 26)
                    easter.day = 19;
                else if ((easter.day == 25) && (d == 28) && (a > 10))
                    easter.day = 18;
            }

            return easter;            
        }

        DateOnly Calendar::getEaster()
        {
            const uint16_t year = openknx.time.getLocalTime().year;
            if (_easter.year != year)
            {
                _easter = calcEaster(year);
            }
            return _easter;
        }

        DateOnly Calendar::calcForthAdvent(uint16_t year)
        {
            tm fourthAdvent = {0};
            fourthAdvent.tm_year = year - 1900;
            fourthAdvent.tm_mon = 11;
            fourthAdvent.tm_mday = 24;
            fourthAdvent.tm_hour = 12;
            fourthAdvent.tm_min = 0;
            fourthAdvent.tm_sec = 0;
            fourthAdvent.tm_isdst = 0;
            mktime(&fourthAdvent);
            DateOnly result = {0};
            result.year = year;
            result.month = 12;
            result.day = 24 - fourthAdvent.tm_wday;
            return result;
        }

        DateOnly Calendar::getForthAdvent()
        {
            const uint16_t year = openknx.time.getLocalTime().year;
            if (_fourthAdvent.year != year)
            {
                _fourthAdvent = calcForthAdvent(year);
            }
            return _fourthAdvent;
        }

#ifdef LOG_HolidayKo
        // Functions are currently depending on the logic modul implementation of holiday calculation
        bool Calendar::isHolidayToday()
        {
            return Timer::instance().holidayToday();
        }

        bool Calendar::isHolidayTomorrow()
        {
            return Timer::instance().holidayTomorrow();
        }

        bool Calendar::isWorkingDayToday()
        {
            int wday = openknx.time.getLocalTime().dayOfWeek;
            return wday > 0 && wday < 6 && // Monday to Friday
                   !isHolidayToday();
        }

        bool Calendar::isWorkingDayTomorrow()
        {
            int wday = openknx.time.getLocalTime().dayOfWeek;
            return wday >= 0 && wday < 5 && // Sunday to Thuersday
                   !isHolidayTomorrow();
        }
#endif

    } // namespace Time
} // namespace OpenKNX