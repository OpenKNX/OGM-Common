#include "OpenKNX.h"

VersionCheckResult OpenKNX::versionCheck(uint16_t manufacturerId, uint8_t *hardwareType, uint16_t firmwareVersion)
{
    VersionCheckResult check = FlashAllInvalid;
    if (manufacturerId == 0x00FA)
    {
        // hardwareType has the format 0x00 00 Ap nn vv 00
        if (memcmp(knx.bau().deviceObject().hardwareType(), hardwareType, 4) == 0)
        {
            check = FlashTablesInvalid;
            if (knx.bau().deviceObject().hardwareType()[4] == hardwareType[4])
            {
                check = FlashValid;
            }
            else
            {
                println("ApplicationVersion changed, ETS has to reprogram the application!");
            }
        }
    }
    else
    {
        println("This firmware supports only applicationId 0x00FA");
    }
    return check;
}

void OpenKNX::knxRead(uint8_t openKnxId, uint8_t applicationNumber, uint8_t applicationVersion, uint8_t firmwareRevision)
{
    uint8_t hardwareType[LEN_HARDWARE_TYPE] = {0x00, 0x00, openKnxId, applicationNumber, applicationVersion, 0x00};

    // first setup flash version check
    knx.bau().versionCheckCallback(versionCheck);
    // set correct hardware type for flash compatibility check
    knx.bau().deviceObject().hardwareType(hardwareType);
    // read flash data
    knx.readMemory();
    // set hardware type again, in case an other hardware type was deserialized from flash
    knx.bau().deviceObject().hardwareType(hardwareType);
    // set firmware version als user info (PID_VERSION)
    // 5 bit major, 5 bit minor, 6 bit revision
    knx.bau().deviceObject().version(((applicationVersion & 0xF0) << 7) | ((applicationVersion & 0xF) << 6) | (firmwareRevision & 0x3F));
}
