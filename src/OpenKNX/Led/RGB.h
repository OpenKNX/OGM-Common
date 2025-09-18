#pragma once
#include "OpenKNX/Led/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        enum Color
        {
            Red = 0xff0000,
            Green = 0x00ff00,
            Blue = 0x000000
        };

        class RGB : public Base
        {
          public:
            virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;
            virtual void setColor(uint32_t rgb);
            /*
             * Return if led is capable of RGB colors
             */
            virtual bool isRGB() { return true; }
        };
    } // namespace Led
} // namespace OpenKNX