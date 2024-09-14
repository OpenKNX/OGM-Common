#include "OpenKNX/Led/Effects/Blink.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
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

            uint8_t __time_critical_func(Blink::value)(uint8_t maxValue)
            {
                // if (delayCheck(_lastMillis, _frequency) || _lastMillis == 0)
                // {
                //     _state = !_state;
                //     _lastMillis = millis();
                // }

                // return _state ? maxValue : 0;
                return int(maxValue * brightness());
            }

            float __time_critical_func(Blink::brightness)()
            {
                if (delayCheck(_lastMillis, _frequency) || _lastMillis == 0)
                {
                    _state = !_state;
                    _lastMillis = millis();
                }

                return _state ? 1.0 : 0.0;
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX