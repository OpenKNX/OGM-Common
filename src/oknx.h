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
};

extern OpenKNXfacade openknx;