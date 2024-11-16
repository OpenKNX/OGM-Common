#pragma once
#include "time.h"

namespace OpenKNX
{
    namespace Time
    {
        class Calendar
        {
            tm _easter = {0};
            tm _fourthAdvent = {0};
          public:
            /*
             * Returns true, if the calculation is valid
             */
            bool isValid();
            /*
             * get easter
             */
            tm getEaster();
            /*
             * get 4th advent
             */
            tm getForthAdvent();
        };
    } // namespace Time
} // namespace OpenKNX