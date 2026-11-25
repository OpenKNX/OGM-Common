#pragma once

#ifdef USE_DATASECURE

#include <string>

namespace OpenKNX
{
    namespace Fdsk
    {
        // FDSK-String wie auf dem KNX-Secure-Zertifikat/Aufkleber und wie ihn die ETS
        // beim Hinzufügen eines Secure-Geräts erwartet: 6 Byte KNX-Seriennummer +
        // 16 Byte FDSK + CRC-4 (High-Nibble eines 23. Bytes), Base32, 36 Zeichen in
        // 6 Gruppen zu 6 Zeichen, getrennt durch '-'.
        // Muss mit OAM-DaliGateway/scripts/Generate-Secure-Certificate.ps1 übereinstimmen.
        std::string getCertificate();
    } // namespace Fdsk
} // namespace OpenKNX

#endif // USE_DATASECURE
