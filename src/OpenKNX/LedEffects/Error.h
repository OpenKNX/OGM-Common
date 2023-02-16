#pragma once
#include "OpenKNX/LedEffects/Base.h"
#include "knx.h"

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
            void init(uint8_t code);
            bool value();
        };
    } // namespace LedEffects
} // namespace OpenKNX