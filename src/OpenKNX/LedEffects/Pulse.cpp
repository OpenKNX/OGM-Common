#include "OpenKNX/LedEffects/Pulse.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        uint16_t Pulse::curve(uint16_t value)
        {
            return (value <= _frequencyHalf) ? value : (_frequencyHalf - (value - _frequencyHalf));
        }

        uint8_t Pulse::mapping(uint16_t value)
        {
            return _table[(value * _tableSize / _frequencyHalf)];
        }

        uint8_t Pulse::value(uint32_t micros /* = 0 */, uint8_t maxValue /* = 255 */)
        {
            // first run
            if (_lastMicros == 0)
                _lastMicros = micros;

            uint16_t value = curve(((micros - _lastMicros) / 1000) % _frequency);
            uint8_t table = mapping(value);

            return (uint8_t)round((float)table * maxValue / 255);
        }

        void Pulse::init(uint16_t frequency)
        {
            Base::init();
            _frequency = frequency;
            _frequencyHalf = frequency / 2;
        }
    } // namespace LedEffect
} // namespace OpenKNX