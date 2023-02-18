#include "OpenKNX/LedEffects/Pulse.h"

namespace OpenKNX
{
    namespace LedEffects
    {
#ifdef __time_critical_func
        uint8_t __time_critical_func(Pulse::value)(uint8_t maxValue /* = 255 */)
#else
        uint8_t Pulse::value(uint8_t maxValue /* = 255 */)
#endif
        {
            // first run
            if (_lastMillis == 0)
                _lastMillis = millis();

            uint8_t value = _table[((millis() - _lastMillis) % (_frequency * 2) * 46 / (_frequency * 2))];

            if (maxValue == 255)
            {
                return value;
            }
            else
            {
                return value * maxValue / 255;
            }
        }

        void Pulse::init(uint16_t frequency)
        {
            Base::init();
            _frequency = frequency;
        }
    } // namespace LedEffects
} // namespace OpenKNX