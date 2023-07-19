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
            volatile uint16_t _frequency = OPENKNX_LEDEFFECT_BLINK_FREQ;

            bool _state = false;

          public:
            /*
             * call value
             */
            bool value();

            void init();
            void init(uint16_t frequency);
        };
    } // namespace LedEffects
} // namespace OpenKNX