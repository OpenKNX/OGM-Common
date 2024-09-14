#include "OpenKNX/Led/Effects/Activity.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            uint16_t __time_critical_func(Activity::value)(uint16_t maxValue)
            {
                // if (_lastActivity >= _lastMillis && delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION + OPENKNX_LEDEFFECT_ACTIVITY_PAUSE))
                //     _lastMillis = millis();

                // if (_inverted)
                //     return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_PAUSE) ? maxValue : 0;
                // else
                //     return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION) ? 0 : maxValue;
                return int(maxValue * brightness());
            }

            float __time_critical_func(Activity::brightness)()
            {
                if (_lastActivity >= _lastMillis && delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION + OPENKNX_LEDEFFECT_ACTIVITY_PAUSE))
                    _lastMillis = millis();

                if (_inverted)
                    return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_PAUSE) ? 1.0 : 0.0;
                else
                    return delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION) ? 0.0 : 1.0;
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX