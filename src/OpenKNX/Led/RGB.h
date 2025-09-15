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
            /*
             * Return if led is capable of RGB colors
             */
            virtual bool isRGB() { return true; }
        };
    } // namespace Led
} // namespace OpenKNX