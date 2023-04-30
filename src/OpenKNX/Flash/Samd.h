#pragma once
#ifdef ARDUINO_ARCH_SAMD

#include "OpenKNX/Flash/Base.h"

namespace OpenKNX
{
    namespace Flash
    {
        class Samd : public Base
        {
          protected:
          public:
            Samd(uint32_t startOffset, uint32_t size, std::string id);
            void eraseSector(uint16_t sector = 0) override;
            void writeSector() override;
            uint8_t* flash() override;
        };
    } // namespace Flash
} // namespace OpenKNX
#endif
