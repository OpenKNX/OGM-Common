#include "OpenKNX/LedEffects/Blink.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        bool Blink::value(uint32_t micros /* = 0 */)
        {
            // first run start with ON
            if (_lastMicros == 0)
            {
                _lastMicros = micros;
                _state = true;
                return true;
            }

            // return with current state
            if (!delayCheck(micros, (_frequency * 1000)))
                return _state;

            // swtiche led & reset rimer
            _lastMicros = micros;
            _state = !_state;

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