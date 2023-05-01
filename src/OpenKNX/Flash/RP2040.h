#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include "OpenKNX/Flash/Base.h"

namespace OpenKNX
{
    namespace Flash
    {
        class RP2040 : public Base
        {
          protected:
          public:
            RP2040(uint32_t offset, uint32_t size, std::string id);
            void eraseSector(uint16_t page = 0) override;
            void writeSector() override;
            uint8_t* flash() override;
        };

    } // namespace Flash
} // namespace OpenKNX
#endif