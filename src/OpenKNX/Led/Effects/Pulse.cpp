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

            uint8_t __time_critical_func(Pulse::value)()
            {
                // first run
                if (_lastMillis == 0) _lastMillis = millis();

                constexpr uint8_t refval = 255 - OPENKNX_LEDEFFECT_PULSE_MIN;
                return (0.5 * (1 + sin(PI * ((millis() - _lastMillis) % (_frequency * 2)) / 1000.0)) * refval) + OPENKNX_LEDEFFECT_PULSE_MIN;
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX