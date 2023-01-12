#include "oknx.h"

OpenKNXfacade openknx;

FlashUserData* OpenKNXfacade::flashUserData() 
{
    return _flashUserDataPtr; 
}

void OpenKNXfacade::loop() {
    _flashUserDataPtr->loop();
    knx.loop();
}

void OpenKNXfacade::readMemory(uint8_t openKnxId, uint8_t applicationNumber, uint8_t applicationVersion, uint8_t firmwareRevision, const char* OrderNo /*= nullptr*/)
{
    OpenKNX::knxRead(openKnxId, applicationNumber, applicationVersion, firmwareRevision, OrderNo);
}

