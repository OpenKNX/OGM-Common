#pragma once
#include "OpenKNX/Led/Effects/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {

            class Flash : public Base
            {
              protected:
                volatile uint16_t _duration = 0;
                volatile uint16_t _repeatCycleTime = 0;
                volatile uint8_t _count = 1;

                bool _state = false;

              public:
                /**
                 * Flash effect - produces one or more short blinks.
                 *
                 * The flash duration is defined by OPENKNX_LEDEFFECT_FLASH_DURATION (ms).
                 * The gap between flashes is defined by OPENKNX_LEDEFFECT_FLASH_GAP (ms).
                 *
                 * @param count            Number of flashes per cycle (default 1)
                 * @param repeatCycleTime  Fixed cycle length (ms) after which the pattern restarts.
                 *                         0 = no repeat, the burst plays once and stays off.
                 *                         If the burst is longer than repeatCycleTime, it gets
                 *                         cut short and restarts immediately.
                 *
                 * Examples:
                 *   Flash()              -> single blink, then off forever
                 *   Flash(2)             -> double blink, then off forever
                 *   Flash(2, 1000)       -> double blink every 1000ms
                 *   Flash(3, 1000)       -> triple blink every 1000ms
                 *
                 * Two effects with the same repeatCycleTime and creation time
                 * will always start their bursts in sync.
                 */
                Flash(uint8_t count = 1, uint16_t repeatCycleTime = 0);
                ~Flash() {};
                uint8_t value() override;
            };
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX