#pragma once
#include "OpenKNX/LedEffects/Base.h"
#include "knx.h"

#ifndef OPENKNX_LEDEFFECT_PULSE_FREQ
#define OPENKNX_LEDEFFECT_PULSE_FREQ 1000
#endif

namespace OpenKNX
{
    namespace LedEffects
    {

        class Pulse : public Base
        {
          protected:
            uint16_t _frequency = OPENKNX_LEDEFFECT_PULSE_FREQ;
            uint16_t _frequencyHalf = OPENKNX_LEDEFFECT_PULSE_FREQ / 2;

            // use a table for a consistent pulsing
            const uint8_t _tableSize PROGMEM = 24;
            const uint8_t _table[24] PROGMEM = {5, 6, 7, 8, 10, 11, 13, 16, 19, 23, 27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255};
            // const uint8_t _table[32] PROGMEM =
            //     {0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
            //      27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255};

            uint16_t curve(uint16_t value);
            uint8_t mapping(uint16_t value);

          public:
            /*
             * call value based on micros
             */
            uint8_t value(uint32_t micros = 0, uint8_t maxValue = 255);
            /*
             * Reset internal micros to start first sequence and give a frequency for effect
             */
            void init(uint16_t frequency);
        };
    } // namespace LedEffects
} // namespace OpenKNX