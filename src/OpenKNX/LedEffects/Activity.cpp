#include "OpenKNX/LedEffects/Activity.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        uint8_t __time_critical_func(Activity::value)(uint8_t maxValue)
        {
            if (_lastActivity >= _lastMillis && delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION + OPENKNX_LEDEFFECT_ACTIVITY_PAUSE))
            {
                _lastMillis = millis();
            }

            return !delayCheck(_lastMillis, OPENKNX_LEDEFFECT_ACTIVITY_DURATION) ? maxValue : 0;
        }
    } // namespace LedEffects
} // namespace OpenKNX