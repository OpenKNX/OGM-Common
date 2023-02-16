#include "OpenKNX/LedEffects/Pulse.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        uint16_t Pulse::curve(uint16_t value)
        {
            return (value <= _frequency) ? value : (_frequency - (value - _frequency));
        }

        uint8_t Pulse::mapping(uint16_t value)
        {
            return _table[(value * _tableSize / _frequency)];
        }

        uint8_t Pulse::value(uint8_t maxValue /* = 255 */)
        {
            // first run
            if (_lastMillis == 0)
                _lastMillis = millis();

            uint16_t value = curve((millis() - _lastMillis) % (_frequency * 2));
            uint8_t table = mapping(value);

            return (uint8_t)round((float)table * maxValue / 255);
        }

        void Pulse::init(uint16_t frequency)
        {
            Base::init();
            _frequency = frequency;
        }
    } // namespace LedEffect
} // namespace OpenKNX