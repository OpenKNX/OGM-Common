#include "OpenKNX/LedEffects/Blink.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        bool Blink::value()
        {
            if (DelayCheck(_lastMillis, _frequency) || _lastMillis == 0)
            {
                _state = !_state;
                _lastMillis = millis();
            }

            return _state;
        }

        void Blink::init()
        {
            Base::init();
            _state = false;
            _frequency = OPENKNX_LEDEFFECT_BLINK_FREQ;
        }

        void Blink::init(uint16_t frequency)
        {
            Base::init();
            _state = false;
            _frequency = frequency;
        }
    } // namespace LedEffects
} // namespace OpenKNX