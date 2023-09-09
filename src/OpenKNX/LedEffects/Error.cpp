#include "OpenKNX/Facade.h"
#include "OpenKNX/LedEffects/Blink.h"

namespace OpenKNX
{
    namespace LedEffects
    {
#ifdef __time_critical_func
        bool __time_critical_func(Error::value)()
#else
        bool Error::value()
#endif
        {
            if (
                (delayCheck(_lastMillis, 250) && _counter < _code) ||   // Blink
                (delayCheck(_lastMillis, 1500) && _counter >= _code) || // Pause between sequence
                _lastMillis == 0)
            {
                // Reset
                if (!_state && _counter >= _code)
                    _counter = 0;

                _state = !_state;
                _lastMillis = millis();

                if (!_state)
                    _counter++;
            }

            return _state;
        }

        void Error::init(uint8_t code)
        {
            Base::init();
            _code = code;
            _counter = 0;
        }
    } // namespace LedEffects
} // namespace OpenKNX