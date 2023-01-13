#include "FlashUserData.h"

#include <string.h>
#include "knx.h"
#include "hardware.h"
#include "Helper.h"
#include "HardwareDevices.h"

// singleton
FlashUserData *FlashUserData::_this = nullptr;

FlashUserData::FlashUserData()
{
    // this is a singleton with backreference for static callbacks
    _this = this;
    // knx.beforeRestartCallback(onBeforeRestartHandler);
    // TableObject::beforeTablesUnloadCallback(onBeforeTablesUnloadHandler);
    _flashStart = knx.platform().getNonVolatileMemoryStart();
}

FlashUserData::~FlashUserData()
{}

bool FlashUserData::readFlash()
{
    printDebug("read UserData from flash...\n");
    bool lResult = true;
    // determine size of data to read from flash
    size_t userFlashSize = 0;
    IFlashUserData* next = _first;
    while (next)
    {
        userFlashSize += next->saveSize();
        next = next->next();
    }
    if (userFlashSize > 0 && _flashStart != nullptr)
    {
        size_t flashSize = knx.platform().getNonVolatileMemorySize();
        _userFlashStartRelative = flashSize - userFlashSize - USERDATA_METADATA_SIZE;
    }
    if (_userFlashStartRelative == 0)
    {
        printDebug("no flash for UserData available\n");
        lResult = false;
    }
    uint8_t magicWord[USERDATA_METADATA_SIZE];
    const uint8_t* buffer;
    if (lResult)
    {
        buffer = _flashStart + _userFlashStartRelative;
        buffer = popByteArray(magicWord, USERDATA_METADATA_SIZE, buffer);

        for (uint8_t i = 0; i < 4; i++)
            lResult = lResult && (magicWord[i] == _magicWord[i]);
    }
    if (lResult)
    {
        next = _first;
        while (next)
        {
            const uint8_t* start = buffer;
            buffer = next->restore(buffer);
            printDebug("%s (%i bytes)\n", next->name(), buffer - start);
            next = next->next();
        }
        printDebug("restored UserData\n");
    }
    else
        printDebug("no valid UserData found in flash\n");

// #ifdef SAVE_INTERRUPT_PIN
//     // we need to do this as late as possible, tried in constructor, but this doesn't work on RP2040
//     static bool sSaveInterruptAttached = false;
//     if (!sSaveInterruptAttached)
//     {
//         printDebug("Save interrupt pin attached...\n");
//         pinMode(SAVE_INTERRUPT_PIN, INPUT);
//         attachInterrupt(digitalPinToInterrupt(SAVE_INTERRUPT_PIN), onSafePinInterruptHandler, FALLING);
//     }
//     sSaveInterruptAttached = true;
// #endif
    return lResult;
}

void FlashUserData::writeFlash(const char* debugText)
{
    printDebug("%s", debugText);
    if (knx.configured() && _userFlashStartRelative > 0) 
    {
        // we have to ensure, that writeFlash is not called too often, because flash memory 
        // does not survive too many writes
        // possible sources for many writes: bouncing of SAVE_PIN, call cascades of beforeTablesUnload and beforeRestart,
        // and even touching of the NCN5120 may cause multiple SAVE-Interrupts.
        if (_writeLastCalled == 0 || delayCheck(_writeLastCalled, 180000)) // 3 Minutes delay
        {  
            printDebug("... and executed\n");
            _writeLastCalled = delayTimerInit(); 
            // first get the necessary size of the writeBuffer
            uint16_t writeBufferSize = _metadataSize;
            IFlashUserData* next = _first;
            while (next)
            {
                writeBufferSize = MAX(writeBufferSize, next->saveSize());
                next = next->next();
            }
            uint8_t buffer[writeBufferSize];
            uint32_t flashPos = _userFlashStartRelative;
            uint8_t* bufferPos = buffer;

            if (_metadataSize > 0) 
            {
                // currently we write just a magic word, there are also examples how we would write other metadata
                // bufferPos = pushWord(_deviceObject.apiVersion, bufferPos);
                // bufferPos = pushWord(_deviceObject.manufacturerId(), bufferPos);
                bufferPos = pushByteArray(_magicWord, USERDATA_METADATA_SIZE, bufferPos);
                // bufferPos = pushWord(_deviceObject.version(), bufferPos);

                flashPos = writeFlash(flashPos, bufferPos - buffer, buffer);
            }
            printDebug("saving FlashUserData...\n");
            next = _first;
            while (next)
            {
                bufferPos = next->save(buffer);
                printDebug("%s (size req: %i, act: %i)\n", next->name(), next->saveSize(), bufferPos - buffer);
                flashPos = writeFlash(flashPos, bufferPos - buffer, buffer);
                next = next->next();
            }
            saveFlash();
            printDebug("UserData written to flash, this took %i ms\n", millis() - _writeLastCalled);
        }
        else
        {
            printDebug("... but not executed due to repeated calls to writeFlash() within 3 minutes\n");
        }
    }
    else
    {
        printDebug("... but not executed due to missing configuration data\n");
    }
}

void FlashUserData::saveFlash()
{
    knx.platform().commitNonVolatileMemory();
}

void FlashUserData::first(IFlashUserData* obj)
{
    if (_first != 0)
        obj->next(_first);
    _first = obj;
}

IFlashUserData* FlashUserData::first()
{
    return _first;
}

uint32_t FlashUserData::writeFlash(uint32_t relativeAddress, size_t size, uint8_t* data)
{
    return knx.platform().writeNonVolatileMemory(relativeAddress, data, size);
}

void FlashUserData::onBeforeRestartHandler()
{
    _this->writeFlash("beforeRestartHandler called");
}

void FlashUserData::onBeforeTablesUnloadHandler()
{
    _this->writeFlash("beforeTablesUnload called\n");
}

void FlashUserData::onSafePinInterruptHandler()
{
    printDebug("savePinInterruptHandler called\n");
    _this->_saveInterruptHandlerCalled = true;
}

void FlashUserData::loop()
{
    processSaveInterrupt();
}

void FlashUserData::processSaveInterrupt()
{
    if (_saveInterruptHandlerCalled)
    {
        // debounce and prevent additional interrupts during save execution
        // turn off power consuming devices
        savePower();
        // let each module turn off its power consuming devices
        IFlashUserData* next = _this->_first;
        while (next)
        {
            next->powerOff();
            next = next->next();
        }
        printDebug("all modules turned power off\n");
        // write all userdata to flash
        _this->writeFlash("writeFlash called");
        printDebug("\n");
        // in case it was a jitter on the SAVE-Pin, we restore power after save

        restorePower();
        next = _this->_first;
        bool noReboot = true;
        while (next && noReboot)
        {
            noReboot = noReboot && next->powerOn();
            next = next->next();
        }
        if (noReboot)
            printDebug("\nall modules restored power\n");
        else
            knx.platform().restart();
        _saveInterruptHandlerCalled = false;
        printDebug("SaveInterrupt was handled correctly\n");

    }
}
