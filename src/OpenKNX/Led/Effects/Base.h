#pragma once
#include "Arduino.h"
#include "OpenKNX/Helper.h"
#include "OpenKNX/defines.h"

namespace OpenKNX
{

    namespace Led
    {
        namespace Effects
        {
            class Base
            {
              protected:
                volatile uint32_t _lastMillis = 0;

              public:
                virtual uint16_t value(uint16_t maxValue) = 0;
                virtual float brightness() = 0;
                virtual ~Base() {};
            };
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX