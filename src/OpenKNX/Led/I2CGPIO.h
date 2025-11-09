#pragma once
#include "OpenKNX/Led/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        class I2CGPIO : public Base
        {
          private:
            volatile long _activeOn = HIGH;
            volatile long _pin = -1;

            void writeLed(uint8_t brightness) override;

          public:
            I2CGPIO(long pin = -1, long activeOn = HIGH);
            void init() override;
            bool isDimmable() override;

            long getPin() { return _pin; }
        };
    } // namespace Led
} // namespace OpenKNX