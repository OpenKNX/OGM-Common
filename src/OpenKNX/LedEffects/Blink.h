#pragma once
#include "OpenKNX/LedEffects/Base.h"
#include "knx.h"

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
            uint16_t _frequency = OPENKNX_LEDEFFECT_BLINK_FREQ;

            bool _state = false;
            /*
             * Return frequency in micros
             */
            uint32_t frequencyMicros();

          public:
            /*
             * call value based on micros
             */
            bool value(uint32_t micros = 0);

            void init();
            void init(uint16_t frequency);
        };
    } // namespace LedEffects
} // namespace OpenKNX