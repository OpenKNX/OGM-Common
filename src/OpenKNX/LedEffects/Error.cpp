#include "OpenKNX/Facade.h"
#include "OpenKNX/LedEffects/Blink.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        Error::Error(uint8_t code)
        {
            _code = code;
        }

        uint8_t __time_critical_func(Error::value)(uint8_t maxValue)
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

            return _state ? maxValue : 0;
        }
    } // namespace LedEffects
} // namespace OpenKNX