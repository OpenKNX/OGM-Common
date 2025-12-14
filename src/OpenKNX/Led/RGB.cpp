#include "OpenKNX/Led/RGB.h"

namespace OpenKNX
{
    namespace Led
    {
        void RGB::color(uint32_t rgb)
        {
            uint8_t r = (rgb >> 16) & 0xFF;
            uint8_t g = (rgb >> 8) & 0xFF;
            uint8_t b = rgb & 0xFF;
            color(r, g, b);
        }

        void RGB::color(Color value)
        {
            color(static_cast<uint32_t>(value));
        }
    } // namespace Led
} // namespace OpenKNX