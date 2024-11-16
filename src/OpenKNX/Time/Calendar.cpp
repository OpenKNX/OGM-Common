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

        tm Calendar::getEaster()
        {
            tm localTime = openknx.time.getLocalTime();
            if (_easter.tm_year != localTime.tm_year)
            {
                _easter = localTime;
                // calculate easter
                uint16_t lYear = localTime.tm_year + 1900;
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
                    _easter.tm_mday = 22 + d + e;
                    _easter.tm_mon = 3 - 1;
                }
                else
                {
                    _easter.tm_mday = d + e - 9;
                    _easter.tm_mon = 4 - 1;

                    // handle two exceptions
                    if (_easter.tm_mday == 26)
                        _easter.tm_mday = 19;
                    else if ((_easter.tm_mday == 25) && (d == 28) && (a > 10))
                        _easter.tm_mday = 18;
                }
            }
            return _easter;
        }

        tm Calendar::getForthAdvent()
        {
            tm localTime = openknx.time.getLocalTime();
            if (_fourthAdvent.tm_year != localTime.tm_year)
            {
                _fourthAdvent = localTime;
                _fourthAdvent.tm_mon = 11;
                _fourthAdvent.tm_mday = 24;
                _fourthAdvent.tm_hour = 12;
                _fourthAdvent.tm_min = 0;
                _fourthAdvent.tm_sec = 0;
                mktime(&_fourthAdvent); //   -timezone;
                _fourthAdvent.tm_mday = 24 - _fourthAdvent.tm_wday;
                _fourthAdvent.tm_mon = 12 - 1;
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
            int wday = openknx.time.getLocalTime().tm_wday;
            return wday > 0 && wday < 6 && // Monday to Friday
                   !isHolidayToday();
        }

        bool Calendar::isWorkingDayTommorow()
        {
            int wday = openknx.time.getLocalTime().tm_wday;
            return wday >= 0 && wday < 5 && // Sunday to Thuersday
                   !isHolidayTommorow();
        }
#endif

    } // namespace Time
} // namespace OpenKNX