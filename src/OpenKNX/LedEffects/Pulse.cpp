#include "OpenKNX/LedEffects/Pulse.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        Pulse::Pulse(uint16_t frequency)
        {
            _frequency = frequency;
        }

#ifdef __time_critical_func
        uint8_t __time_critical_func(Pulse::value)(uint8_t maxValue /* = 255 */)
#else
        uint8_t Pulse::value(uint8_t maxValue /* = 255 */)
#endif
        {
            // first run
            if (_lastMillis == 0) _lastMillis = millis();

            // calc
            uint8_t value = _ledPulseEffetTable[((millis() - _lastMillis) % (_frequency * 2) * 46 / (_frequency * 2))];

            return (maxValue == 255) ? value : value * maxValue / 255;
        }
    } // namespace LedEffects
} // namespace OpenKNX