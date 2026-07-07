#include "OpenKNX/Led/Effects/Flash.h"

#ifndef OPENKNX_LEDEFFECT_FLASH_DURATION
    #define OPENKNX_LEDEFFECT_FLASH_DURATION 50
#endif
#ifndef OPENKNX_LEDEFFECT_FLASH_GAP
    #define OPENKNX_LEDEFFECT_FLASH_GAP 100
#endif

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            Flash::Flash(uint8_t count, uint16_t repeatCycleTime)
            {
                _count = count > 0 ? count : 1;
                _repeatCycleTime = repeatCycleTime;
                _lastMillis = millis();
            }

            uint8_t __time_critical_func(Flash::value)()
            {
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

                // One flash slot = DURATION + GAP
                // Each slot: [ON for DURATION] [OFF for GAP]
                // The last flash has a trailing GAP too, which just becomes part of the pause.
                uint32_t flashCycle = OPENKNX_LEDEFFECT_FLASH_DURATION + OPENKNX_LEDEFFECT_FLASH_GAP;

                // Total burst length: count full slots
                // Example count=3: ON(50) GAP(100) ON(50) GAP(100) ON(50) GAP(100) = 450ms
                uint32_t burstLength = _count * flashCycle;

                if (elapsed < burstLength)
                {
                    // Position within current slot: ON during DURATION, OFF during GAP
                    uint32_t posInSlot = elapsed % flashCycle;
                    _state = (posInSlot < OPENKNX_LEDEFFECT_FLASH_DURATION);
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