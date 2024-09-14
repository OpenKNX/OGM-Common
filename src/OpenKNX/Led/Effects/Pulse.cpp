#include "OpenKNX/Led/Effects/Pulse.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            Pulse::Pulse(uint16_t frequency)
            {
                _frequency = frequency;
            }

            uint16_t __time_critical_func(Pulse::value)(uint16_t maxValue /* = 255 */)
            {
                // first run
                // if (_lastMillis == 0) _lastMillis = millis();
                // uint32_t ref_ms = millis() - _lastMillis;
                // uint32_t duration_ms = _frequency * 2;
                // uint32_t t = ref_ms % duration_ms;

                // calc
                //              uint8_t value = (t % (_frequency * 2) * 46 / (_frequency * 2));
                //             return t % (_frequency * 2)  //;   / (_frequency * 2);

                // uint8_t value = _ledPulseEffetTable[(t % (_frequency * 2) * 46 / (_frequency * 2))];
                // return (maxValue == 255) ? value : value * maxValue / 255;

                // float brightness = 0.5 * (1 + sin(PI * t / 1000.0));  // Normierte Helligkeit zwischen 0 und 1
                // return int(brightness * maxValue);

                // float brightness = t <= 1000 ? (float)t / 1000.0 : 1.0 - (float)(t - 1000) / 1000.0;
                // return = int(brightness * (maxValue - 7)) + 7;

                return int(maxValue * brightness());
            }

            float __time_critical_func(Pulse::brightness)()
            {
                // first run
                if (_lastMillis == 0) _lastMillis = millis();
                return 0.5 * (1 + sin(PI * ((millis() - _lastMillis) % (_frequency * 2)) / 1000.0));
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX