#pragma once
#include "OpenKNX/Led/RGB.h"

namespace OpenKNX
{
    namespace Led
    {
        class GPIO_RGB : public RGB
        {
          protected:
            uint8_t _color[3] = {0, 0, 0}; // R, G, B
            long _pins[3] = {-1, -1, -1};  // R, G, B
            volatile long _activeOn = HIGH;
            volatile bool _colorDirty = false;

          protected:
            void writeLed(uint8_t brightness) override;

          public:
            GPIO_RGB(long pin_r, long pin_g, long pin_b, long activeOn, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
            GPIO_RGB(long pin_r, long pin_g, long pin_b, long activeOn, Color rgb);
            void init() override;
            virtual void color(uint8_t r, uint8_t g, uint8_t b) override;
        };
    } // namespace Led
} // namespace OpenKNX