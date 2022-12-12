#pragma once

// #include <stdint.h>
#include <stddef.h>
#include "IFlashUserData.h"

class FlashUserData
{
public:
    FlashUserData();
    virtual ~FlashUserData();
    void addFlashUserDataClass(IFlashUserData *obj);
    void writeFlash();
    void readFlash();
    uint32_t writeFlash(uint32_t relativeAddress, size_t size, uint8_t* data);

private:
    IFlashUserData* _flashUserDataClass = 0;
    uint16_t _metadataSize = 0; // currently no metadata necessary, just 
    void saveFlash();
};
