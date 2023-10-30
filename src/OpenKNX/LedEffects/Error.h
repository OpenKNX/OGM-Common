#pragma once
#include "OpenKNX/LedEffects/Base.h"

namespace OpenKNX
{
    namespace LedEffects
    {

        class Error : public Base
        {
          protected:
            uint8_t _code = 1;
            uint8_t _counter = 0;
            bool _state = false;

          public:
            Error(uint8_t code);
            ~Error(){};
            uint8_t value(uint8_t maxValue) override;
        };
    } // namespace LedEffects
} // namespace OpenKNX