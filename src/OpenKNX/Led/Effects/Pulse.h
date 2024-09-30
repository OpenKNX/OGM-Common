#pragma once
#include "OpenKNX/Led/Effects/Base.h"

#ifndef OPENKNX_LEDEFFECT_PULSE_FREQ
    #define OPENKNX_LEDEFFECT_PULSE_FREQ 1000
#endif

#ifndef OPENKNX_LEDEFFECT_PULSE_MIN
    #define OPENKNX_LEDEFFECT_PULSE_MIN 5
#endif

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            class Pulse : public Base
            {
              protected:
                uint16_t _frequency = 0;

              public:
                Pulse(uint16_t frequency = OPENKNX_LEDEFFECT_PULSE_FREQ);
                ~Pulse() {};
                uint8_t value() override;
            };
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX