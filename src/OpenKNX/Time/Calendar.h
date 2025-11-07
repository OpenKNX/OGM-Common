#pragma once
#include "OpenKNX/DateTime.h"
#include "knxprod.h"

namespace OpenKNX
{
    namespace Time
    {
        class Calendar
        {
            DateOnly _easter = {0};
            DateOnly _fourthAdvent = {0};

          public:
            /*
             * Returns true, if the calculation is valid
             */
            bool isValid();

            /*
             * returns the current date based on the UTC time
             */
            DateOnly getUtcDate();

            /*
             * returns the current date based on the local time
             */
            DateOnly getLocalDate();

            /*
             * calc easter
             */
            static DateOnly calcEaster(uint16_t year);

            /*
             * get easter for current year
             */
            DateOnly getEaster();

            /*
             * calc 4th advent
             */
            static DateOnly calcForthAdvent(uint16_t year);

            /*
             * get 4th advent for current year
             */
            DateOnly getForthAdvent();

#ifdef LOG_HolidayKo
            // Working day functions are currently depending on OFM-LogicModule.
            // This will be changed in the future.

            /*
             * Check if today is defined as holiday.
             * @return true, if current date is a holiday enabled in configuration
             */
            bool isHolidayToday();

            /*
             * Check if tomorrow is defined as holiday.
             * @return true, if the next day after current date is a holiday enabled in configuration
             */
            bool isHolidayTomorrow();

            /*
             * Check if today is a working day.
             * @return true, if the day defined by current date is within Monday to Friday and not a holiday enabled in configuration
             */
            bool isWorkingDayToday();

            /*
             * Check if tomorrow is a working day.
             * @return true, if the next day after current date is within Monday to Friday and not a holiday enabled in configuration
             */
            bool isWorkingDayTomorrow();
#endif
        };
    } // namespace Time
} // namespace OpenKNX