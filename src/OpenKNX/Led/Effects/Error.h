#pragma once
#include "OpenKNX/Led/Effects/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        namespace Effects
        {
            class Error : public Base
            {
              protected:
                uint8_t _code = 1;
                uint8_t _counter = 0;
                bool _state = false;

              public:
                Error(uint8_t code);
                ~Error() {};
                uint16_t value(uint16_t maxValue) override;
                float brightness() override;
            };
        } // namespace Effects
    } // namespace Led
} // namespace OpenKNX