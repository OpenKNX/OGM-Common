#pragma once
#include "time.h"
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
             * get easter
             */
            DateOnly getEaster();
            /*
             * get 4th advent
             */
            DateOnly getForthAdvent();

#ifdef LOG_HolidayKo
            // woring day functions are currently depenting on the loglic module. 
            // This will be changed in the future.

            /*
            * returns true if today is a holiday
            */
            bool isHolidayToday();
            /*
            * returns true if the tomorrow is a holiday
            */
            bool isHolidayTommorow();
            /*
            * returns true for days from Monday to Friday if there is no holiday
            */
            bool isWorkingDayToday();
            /*
            * returns true if the tomorrow is a day from Monday to Friday and if there is no holiday
            */
            bool isWorkingDayTommorow();
#endif
        };
    } // namespace Time
} // namespace OpenKNX