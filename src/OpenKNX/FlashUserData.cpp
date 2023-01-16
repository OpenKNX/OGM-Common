#include "OpenKNX/FlashUserData.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    FlashUserData::FlashUserData()
    {
        flashStart = knx.platform().getNonVolatileMemoryStart();
        flashSize = knx.platform().getNonVolatileMemorySize();
    }

    FlashUserData::~FlashUserData()
    {}

    void FlashUserData::load()
    {
        uint32_t start = millis();
        
        // skip if not userdata available
        if (userDataSize() == 0)
            return;

        openknx.debug("FlashUserData", "load (%i bytes)", userDataSize());
        uint8_t *buffer = flashStart + flashSize - userDataSize();

        if (!validateChecksum(buffer, userDataSize()))
        {
            openknx.debug("FlashUserData", "  Global checksum invalid");
            return;
        }

        // load data from modules
        uint16_t size = 0;
        uint16_t currentPosition = 5;
        Modules *modules = openknx.getModules();
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            Module *module = modules->list[i - 1];
            size = module->userDataSize();

            // Module has no user content
            if (size == 0)
                continue;

            // move pointer to correct position
            uint8_t *partialPointer = (buffer + currentPosition - 1);
            openknx.debug("FlashUserData", "  %s (%i bytes)", module->name(), size);
            if (!validateChecksum(partialPointer, size + 1))
            {
                openknx.debug("FlashUserData", "    Checksum for module invalid");
                return;
            }

            modules->list[i - 1]->restoreUserData(partialPointer);
            currentPosition += size + 1; // size + 1 checksum
        }

        openknx.debug("FlashUserData", "  complete (%i)", millis() - start);
    }

    void FlashUserData::save(bool force /* = false */)
    {
        uint32_t start = millis();

        // skip if not userdata available
        if (userDataSize() == 0)
            return;

        // table is not loaded (ets prog running) and save is not possible
        if (!knx.configured())
            return;

        // we have to ensure, that save is not called too often, because flash memory
        // does not survive too many writes
        if (!force && lastWrite > 0 && !delayCheck(lastWrite, FLASH_USER_DATA_WRITE_LIMIT))
            return;

        openknx.debug("FlashUserData", "save <%i>", force);

        // fill data
        uint8_t *userData = newUserData();
        uint16_t currentPosition = 5;
        Modules *modules = openknx.getModules();
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            // move pointer to correct position
            uint8_t *partialPointer = (userData + currentPosition - 1);
            uint16_t moduleSize = modules->list[i - 1]->userDataSize();
            // save data to corret position
            modules->list[i - 1]->saveUserData(partialPointer);
            // write checksum behind moduleData
            userData[currentPosition + moduleSize - 1] = calcChecksum(partialPointer, moduleSize);
            // update position for next module
            currentPosition += moduleSize + 1; // +1 for checksum
        }

        // write checksum to last position.
        userData[userDataSize() - 1] = calcChecksum(userData, userDataSize() - 1); // -1 because without checksum byte

        // write to flash
        knx.platform().writeNonVolatileMemory(flashSize - userDataSize(), userData, userDataSize());
        knx.platform().commitNonVolatileMemory();

        lastWrite = millis();
        openknx.debug("FlashUserData", "  complete (%i)", millis() - start);
    }

    uint8_t *FlashUserData::newUserData()
    {
        uint8_t *userData = new uint8_t[userDataSize()];
        memset(userData, 0, userDataSize());

        // MagicWords
        userData[0] = 0xDA;
        userData[1] = 0x77;
        userData[2] = 0x6E;
        userData[3] = 0x82;

        return userData;
    }

    uint16_t FlashUserData::userDataSize()
    {
        if (cachedUserDataSize > 0)
            return cachedUserDataSize;

        cachedUserDataSize = 5; // 4 MagicWords + 1 Checksum
        Modules *modules = openknx.getModules();
        for (uint8_t i = 1; i <= modules->count; i++)
            cachedUserDataSize += modules->list[i - 1]->userDataSize() + 1;

        return cachedUserDataSize;
    }

    uint8_t FlashUserData::calcChecksum(uint8_t *data, uint16_t len)
    {
        uint8_t sum = 0;

        for (uint16_t i = 0; i < len; i++)
            sum = sum + data[i];

        return sum;
    }

    bool FlashUserData::validateChecksum(uint8_t *data, uint16_t len)
    {
        // openknx.debug("FlashUserData", "validateChecksum %i %i", data[len - 1], calcChecksum(data, len - 1));
        // printHEX("  ", data, len);
        return data[len - 1] == calcChecksum(data, len - 1);
    }
} // namespace OpenKNX