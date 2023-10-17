#include "OpenKNX/LedEffects/Flash.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        Flash::Flash(uint16_t duration)
        {
            _duration = duration;
            _lastMillis = millis();
        }

#ifdef __time_critical_func
        uint8_t __time_critical_func(Flash::value)(uint8_t maxValue)
#else
        uint8_t Flash::value(uint8_t maxValue)
#endif
        {
            _state = !delayCheck(_lastMillis, _duration);
            return _state ? maxValue : 0;
        }
    } // namespace LedEffects
} // namespace OpenKNX