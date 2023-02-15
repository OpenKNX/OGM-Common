#pragma once
#include "knx.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        class Base
        {
          protected:
            uint32_t _lastMicros = 0;
            bool delayCheck(uint32_t micros, uint32_t duration);

          public:
            /*
             * Reset internal micros to start first sequence and give a duration for effect
             */
            virtual void init();
        };
    } // namespace LedEffects
} // namespace OpenKNX