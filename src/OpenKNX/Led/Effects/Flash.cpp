#include "OpenKNX/Led/Effects/Flash.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            Flash::Flash(uint16_t duration)
            {
                _duration = duration;
                _lastMillis = millis();
            }

            uint8_t __time_critical_func(Flash::value)()
            {
                _state = !delayCheck(_lastMillis, _duration);
                return _state ? 255 : 0;
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX