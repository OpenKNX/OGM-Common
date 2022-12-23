#pragma once

// #include <stdint.h>
#include <stddef.h>
#include "IFlashUserData.h"

#define USERDATA_METADATA_SIZE 4

class FlashUserData
{
public:
    FlashUserData();
    virtual ~FlashUserData();
    // first class to call for serialization data
    void first(IFlashUserData *obj);
    IFlashUserData* first();
    bool readFlash();
    void loop();

private:
    // singleton
    static FlashUserData* _this;
    
    uint8_t _magicWord[USERDATA_METADATA_SIZE] = {0xDA, 0x77, 0x6E, 0x82};

    static void onBeforeRestartHandler();
    static void onBeforeTablesUnloadHandler();
    static void onSafePinInterruptHandler();

    void processSaveInterrupt();
    void writeFlash(const char* debugText);
    uint32_t writeFlash(uint32_t relativeAddress, size_t size, uint8_t* data);
    void saveFlash();

    // first class to call for serialization data
    IFlashUserData* _first = 0;
    uint16_t _metadataSize = USERDATA_METADATA_SIZE; // space for magic word at the beginning of flash space for user data
    uint32_t _writeLastCalled = 0;
    size_t _userFlashStartRelative = 0; 
    uint8_t* _flashStart = 0;
    bool _saveInterruptHandlerCalled = false;
};
