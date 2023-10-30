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
            virtual uint8_t value(uint8_t maxValue) = 0;
            virtual ~Base(){};
        };
    } // namespace LedEffects
} // namespace OpenKNX