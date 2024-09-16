#include "OpenKNX/Led/Effects/Activity.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            uint8_t __time_critical_func(Activity::value)()
            {
                if (_lastActivity >= _lastMillis && delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION + OPENKNX_LEDEFFECT_ACTIVITY_PAUSE))
                    _lastMillis = millis();

                if (_inverted)
                    return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_PAUSE) ? 255 : 0;
                else
                    return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION) ? 255 : 1;
            };
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX