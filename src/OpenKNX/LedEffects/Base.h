#pragma once
#include "Arduino.h"
#include "OpenKNX/Helper.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        class Base
        {
          protected:
            volatile uint32_t _lastMillis = 0;

          public:
            /*
             * Reset internal millis to start first sequence and give a duration for effect
             */
            virtual void init();
        };
    } // namespace LedEffects
} // namespace OpenKNX