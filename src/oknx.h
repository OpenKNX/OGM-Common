#pragma once

#include "knx.h"
#include "OpenKNX.h"
#include "FlashUserData.h"

class OpenKNXfacade
{
private:
    FlashUserData* _flashUserDataPtr;
public:
    OpenKNXfacade() : _flashUserDataPtr(new FlashUserData()) {};
    ~OpenKNXfacade() {};

    FlashUserData* flashUserData() 
    {
        return _flashUserDataPtr; 
    }

    void loop() {
        _flashUserDataPtr->loop();
        knx.loop();
    }

    void readMemory(uint8_t openKnxId, uint8_t applicationNumber, uint8_t applicationVersion, uint8_t firmwareRevision, const char* OrderNo = nullptr)
    {
        OpenKNX::knxRead(openKnxId, applicationNumber, applicationVersion, firmwareRevision, OrderNo);
    }

};

extern OpenKNXfacade openknx;