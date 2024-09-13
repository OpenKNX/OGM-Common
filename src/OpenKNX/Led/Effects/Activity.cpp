#include "OpenKNX/Led/Effects/Activity.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            uint8_t __time_critical_func(Activity::value)(uint8_t maxValue)
            {
                if (_lastActivity >= _lastMillis && delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION + OPENKNX_LEDEFFECT_ACTIVITY_PAUSE))
                    _lastMillis = millis();

                if (_inverted)
                    return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_PAUSE) ? maxValue : 0;
                else
                    return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION) ? 0 : maxValue;
            };
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX