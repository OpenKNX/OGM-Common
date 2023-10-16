#include "OpenKNX/LedEffects/Flash.h"

namespace OpenKNX
{
    namespace LedEffects
    {
#ifdef __time_critical_func
        bool __time_critical_func(Flash::value)()
#else
        bool Flash::value()
#endif
        {
            _state = !delayCheck(_lastMillis, _duration);
            return _state;
        }

        void Flash::init()
        {
            Base::init();
            _lastMillis = millis();
            _state = true;
            _duration = OPENKNX_LEDEFFECT_FLASH_DURATION;
        }

        void Flash::init(uint16_t duration)
        {
            Base::init();
            _lastMillis = millis();
            _state = true;
            _duration = duration;
        }
    } // namespace LedEffects
} // namespace OpenKNX