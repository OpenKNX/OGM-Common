#include "OpenKNX/Led/RGB.h"

namespace OpenKNX
{
    namespace Led
    {
        void RGB::setColor(uint32_t rgb)
        {
            uint8_t r = (rgb >> 16) & 0xFF;
            uint8_t g = (rgb >> 8) & 0xFF;
            uint8_t b = rgb & 0xFF;
            setColor(r, g, b);
        }

        void RGB::setColor(Color color)
        {
            setColor(static_cast<uint32_t>(color));
        }
    } // namespace Led
} // namespace OpenKNX