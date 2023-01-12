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

    FlashUserData* flashUserData(); 
    void loop();
    void readMemory(uint8_t openKnxId, uint8_t applicationNumber, uint8_t applicationVersion, uint8_t firmwareRevision, const char* OrderNo = nullptr);
};

extern OpenKNXfacade openknx;