#pragma once
#include <knx.h>

class OpenKNX
{
  public:
    // this function is a replacement for knx.read() and implements version checks for OpenKNX
    // it ensures, that only correct applicatins can be loaded from flash
    static void knxRead(uint8_t openKnxId, uint8_t applicationNumber, uint8_t applicationVersion, uint8_t firmwareRevision, const char* OrderNo = nullptr);

  private:
    // this function is called during load of knx data from flash and cecks for version compatibility
    static VersionCheckResult versionCheck(uint16_t manufacturerId, uint8_t *hardwareType, uint16_t firmwareVersion);
};