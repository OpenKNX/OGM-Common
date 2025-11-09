#pragma once
#include "OpenKNX/Led/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        class GPIO : public Base
        {
          private:
            volatile long _activeOn = HIGH;
            volatile long _pin = -1;
            volatile bool _isDimmable = true;

            void writeLed(uint8_t brightness) override;

          public:
            GPIO(long pin = -1, long activeOn = HIGH, bool isDimmable = true);
            void init() override;

            long getPin() { return _pin; }
        };
    } // namespace Led
} // namespace OpenKNX