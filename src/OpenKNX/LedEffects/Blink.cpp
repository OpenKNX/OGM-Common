#include "OpenKNX/LedEffects/Blink.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    namespace LedEffects
    {
#ifdef __time_critical_func
        bool __time_critical_func(Blink::value)()
#else
        bool Blink::value()
#endif
        {
            if (delayCheck(_lastMillis, _frequency) || _lastMillis == 0)
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