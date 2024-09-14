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

            uint16_t __time_critical_func(Flash::value)(uint16_t maxValue)
            {
                // _state = !delayCheck(_lastMillis, _duration);
                // return _state ? maxValue : 0;
                return int(maxValue * brightness());
            }

            float __time_critical_func(Flash::brightness)()
            {
                _state = !delayCheck(_lastMillis, _duration);
                return _state ? 1.0 : 0.0;
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX