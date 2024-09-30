#pragma once
#include "OpenKNX/Led/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        class GPIO : public Base
        {
          private:
            void writeLed(uint8_t brightness) override;

          public:
            void init(long pin = -1, long activeOn = HIGH);
        };
    } // namespace Led
} // namespace OpenKNX