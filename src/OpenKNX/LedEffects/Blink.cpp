#include "OpenKNX/LedEffects/Blink.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        Blink::Blink(uint16_t frequency)
        {
            _frequency = frequency;
        }

        void Blink::updateFrequency(uint16_t frequency)
        {
            _frequency = frequency;
            _lastMillis = 0;
        }

#ifdef __time_critical_func
        uint8_t __time_critical_func(Blink::value)(uint8_t maxValue)
#else
        uint8_t Blink::value(uint8_t maxValue)
#endif
        {
            if (delayCheck(_lastMillis, _frequency) || _lastMillis == 0)
            {
                _state = !_state;
                _lastMillis = millis();
            }

            return _state ? maxValue : 0;
        }
    } // namespace LedEffects
} // namespace OpenKNX