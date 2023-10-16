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
            volatile uint16_t _duration = OPENKNX_LEDEFFECT_FLASH_DURATION;

            bool _state = false;

          public:
            /*
             * call value
             */
            bool value();

            void init() override;
            void init(uint16_t duration);
        };
    } // namespace LedEffects
} // namespace OpenKNX