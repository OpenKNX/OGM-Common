#pragma once
#include "OpenKNX/LedEffects/Base.h"

#ifndef OPENKNX_LEDEFFECT_FLASH_DURATION
    #define OPENKNX_LEDEFFECT_FLASH_DURATION 50
#endif

namespace OpenKNX
{
    namespace LedEffects
    {

        class Flash : public Base
        {
          protected:
            volatile uint16_t _duration = 0;

            bool _state = false;

          public:
            Flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION);
            uint8_t value(uint8_t maxValue) override;
        };
    } // namespace LedEffects
} // namespace OpenKNX