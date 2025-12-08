#pragma once
#include "OpenKNX/Led/Base.h"

#ifndef OPENKNX_LEDCOLOR_RED
    #define OPENKNX_LEDCOLOR_RED 0xff0000
#endif
#ifndef OPENKNX_LEDCOLOR_GREEN
    #define OPENKNX_LEDCOLOR_GREEN 0x00ff00
#endif
#ifndef OPENKNX_LEDCOLOR_BLUE
    #define OPENKNX_LEDCOLOR_BLUE 0x0000ff
#endif
#ifndef OPENKNX_LEDCOLOR_WHITE
    #define OPENKNX_LEDCOLOR_WHITE 0xffffff
#endif
#ifndef OPENKNX_LEDCOLOR_YELLOW
    #define OPENKNX_LEDCOLOR_YELLOW 0xffff00
#endif
#ifndef OPENKNX_LEDCOLOR_ORANGE
    #define OPENKNX_LEDCOLOR_ORANGE 0xffa500
#endif
#ifndef OPENKNX_LEDCOLOR_PURPLE
    #define OPENKNX_LEDCOLOR_PURPLE 0x800080
#endif
#ifndef OPENKNX_LEDCOLOR_CYAN
    #define OPENKNX_LEDCOLOR_CYAN 0x00ffff
#endif
#ifndef OPENKNX_LEDCOLOR_MAGENTA
    #define OPENKNX_LEDCOLOR_MAGENTA 0xff00ff
#endif

#ifndef OPENKNX_LEDCOLOR_CALIBRATION
    #define OPENKNX_LEDCOLOR_CALIBRATION {255, 255, 255}
#endif

constexpr int OpenKNX_LedColor_Calibration[] = OPENKNX_LEDCOLOR_CALIBRATION;

namespace OpenKNX
{
    namespace Led
    {
        enum class Color : uint32_t
        {
            Red = OPENKNX_LEDCOLOR_RED,
            Green = OPENKNX_LEDCOLOR_GREEN,
            Blue = OPENKNX_LEDCOLOR_BLUE,
            White = OPENKNX_LEDCOLOR_WHITE,
            Yellow = OPENKNX_LEDCOLOR_YELLOW,
            Orange = OPENKNX_LEDCOLOR_ORANGE,
            Purple = OPENKNX_LEDCOLOR_PURPLE,
            Cyan = OPENKNX_LEDCOLOR_CYAN,
            Magenta = OPENKNX_LEDCOLOR_MAGENTA
        };

        class RGB : public Base
        {
          public:
            virtual void color(uint8_t r, uint8_t g, uint8_t b) = 0;
            virtual void color(uint32_t rgb);
            virtual void color(Color color);
            /*
             * Return if led is capable of RGB colors
             */
            virtual bool isColor() { return true; }
        };
    } // namespace Led
} // namespace OpenKNX