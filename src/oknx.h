#pragma once

#include "knx.h"
#include "FlashUserData.h"

class OpenKNXfacade
{
private:
    FlashUserData* _flashUserDataPtr;
public:
    OpenKNXfacade() : _flashUserDataPtr(new FlashUserData())
    {};
    ~OpenKNXfacade() {};

    FlashUserData* flashUserData() 
    {
        return _flashUserDataPtr; 
    }

    void loop() {
        _flashUserDataPtr->loop();
        knx.loop();
    }
};

extern OpenKNXfacade openknx;