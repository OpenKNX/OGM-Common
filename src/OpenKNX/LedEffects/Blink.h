#pragma once
#include "OpenKNX/LedEffects/Base.h"

#ifndef OPENKNX_LEDEFFECT_BLINK_FREQ
    #define OPENKNX_LEDEFFECT_BLINK_FREQ 1000
#endif

namespace OpenKNX
{
    namespace LedEffects
    {
        class Blink : public Base
        {
          protected:
            volatile uint16_t _frequency = 0;

            bool _state = false;

          public:
            Blink(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ);
            ~Blink(){};
            uint8_t value(uint8_t maxValue) override;
            void updateFrequency(uint16_t frequency);
        };
    } // namespace LedEffects
} // namespace OpenKNX