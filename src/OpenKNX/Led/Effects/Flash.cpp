#include "OpenKNX/Led/Effects/Flash.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            Flash::Flash(uint16_t duration, uint8_t count, uint16_t repeatCycleTime)
            {
                _duration = duration;
                _count = count > 0 ? count : 1;
                _repeatCycleTime = repeatCycleTime;
                _lastMillis = millis();
            }

            uint8_t __time_critical_func(Flash::value)()
            {
                // Total time the burst needs: count flashes + (count-1) gaps,
                // each of length _duration.
                // Example count=3, duration=100: ON(100) OFF(100) ON(100) OFF(100) ON(100) = 500ms
                uint32_t burstLength = _count * _duration + (_count - 1) * _duration;

                // Time elapsed since effect was created.
                // millis() is only read here, never written – safe in interrupt context.
                uint32_t elapsed = millis() - _lastMillis;

                // If repeating, wrap elapsed time into the fixed cycle window.
                // This ensures the pattern restarts every repeatCycleTime ms.
                // If the burst exceeds repeatCycleTime, it simply gets cut off.
                if (_repeatCycleTime > 0)
                {
                    elapsed = elapsed % _repeatCycleTime;
                }

                if (elapsed < burstLength)
                {
                    // Inside the burst: alternate on/off every _duration ms
                    uint32_t posInBurst = elapsed % (_duration * 2);
                    _state = (posInBurst < _duration);
                }
                else
                {
                    // Outside the burst: LED off (pause or finished)
                    _state = false;
                }

                return _state ? 255 : 0;
            }
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX