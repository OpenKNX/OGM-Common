#pragma once
#include "OpenKNX/Led/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        class RGB : public Base
        {
          public:
            virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;
        };
    } // namespace Led
} // namespace OpenKNX