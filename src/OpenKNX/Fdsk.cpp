#include "OpenKNX/Fdsk.h"

#ifdef USE_DATASECURE

#include "knx.h"
#include "knx/security_interface_object.h"
#include <cstring>

namespace OpenKNX
{
    namespace Fdsk
    {
        namespace
        {
            // CRC-4, Generatorpolynom x^4 + x + 1
            const uint8_t crc4Table[16] = {0x0, 0x3, 0x6, 0x5, 0xC, 0xF, 0xA, 0x9, 0xB, 0x8, 0xD, 0xE, 0x7, 0x4, 0x1, 0x2};

            uint8_t crc4(const uint8_t* data, size_t len)
            {
                uint8_t c = 0;
                for (size_t i = 0; i < len; i++)
                {
                    c = crc4Table[c ^ (data[i] >> 4)];
                    c = crc4Table[c ^ (data[i] & 0x0F)];
                }
                return c;
            }
        } // namespace

        std::string getCertificate()
        {
            static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

            // Seriennummer + FDSK + CRC-4 im High-Nibble des letzten Bytes
            uint8_t buffer[23] = {};
            memcpy(buffer, knx.bau().deviceObject().propertyData(PID_SERIAL_NUMBER), 6);
            memcpy(buffer + 6, SecurityInterfaceObject::fdsk(), 16);
            buffer[22] = crc4(buffer, 22) << 4;

            // Base32: 23 Byte = 184 Bit -> 37 Zeichen, die ersten 36 werden benutzt
            std::string result;
            result.reserve(41);
            uint16_t bits = 0;
            uint8_t bitsLeft = 0;
            uint8_t chars = 0;
            for (size_t i = 0; i < sizeof(buffer) && chars < 36; i++)
            {
                bits = (bits << 8) | buffer[i];
                bitsLeft += 8;
                while (bitsLeft >= 5 && chars < 36)
                {
                    if (chars > 0 && chars % 6 == 0) result += '-';
                    result += alphabet[(bits >> (bitsLeft - 5)) & 0x1F];
                    bitsLeft -= 5;
                    chars++;
                }
            }
            return result;
        }
    } // namespace Fdsk
} // namespace OpenKNX

#endif // USE_DATASECURE
