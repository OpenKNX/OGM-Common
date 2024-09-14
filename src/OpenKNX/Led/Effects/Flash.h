#pragma once
#include "OpenKNX/Led/Effects/Base.h"

#ifndef OPENKNX_LEDEFFECT_FLASH_DURATION
    #define OPENKNX_LEDEFFECT_FLASH_DURATION 50
#endif

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

                bool _state = false;

              public:
                Flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION);
                ~Flash() {};
                uint8_t value(uint8_t maxValue) override;
                float brightness() override;
            };
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX