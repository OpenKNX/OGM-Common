#include "OpenKNX/Facade.h"
#include "OpenKNX/Led/Effects/Blink.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            Error::Error(uint8_t code)
            {
                _code = code;
            }

            uint8_t __time_critical_func(Error::value)()
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

                return _state ? 255 : 0;
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX