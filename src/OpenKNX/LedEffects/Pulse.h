#pragma once
#include "OpenKNX/LedEffects/Base.h"

#ifndef OPENKNX_LEDEFFECT_PULSE_FREQ
    #define OPENKNX_LEDEFFECT_PULSE_FREQ 1000
#endif

const uint8_t _ledPulseEffetTable[48] PROGMEM = {
    5, 6, 7, 8, 10, 11, 13, 16, 19, 23, 27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255,
    255, 215, 181, 152, 128, 108, 91, 76, 64, 54, 45, 38, 32, 27, 23, 19, 16, 13, 11, 10, 8, 7, 6, 5};

namespace OpenKNX
{
    namespace LedEffects
    {

        class Pulse : public Base
        {
          protected:
            uint16_t _frequency = 0;

          public:
            Pulse(uint16_t frequency = OPENKNX_LEDEFFECT_PULSE_FREQ);
            uint8_t value(uint8_t maxValue) override;
        };
    } // namespace LedEffects
} // namespace OpenKNX