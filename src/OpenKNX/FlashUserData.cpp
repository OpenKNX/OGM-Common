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
        openknx.debug("FlashUserData", "load");
        uint32_t start = millis();
        uint8_t *currentPosition;
        uint8_t moduleId = 0;
        uint16_t moduleSize = 0;
        uint16_t userDataSize = 0;
        uint16_t userDataProcessed = 0;
        Module *module = nullptr;

        // check magicwords exists
        currentPosition = flashStart + flashSize - FLASH_USER_DATA_MAGICWORD_LEN;
        if (currentPosition[0] != 0xDA || currentPosition[1] != 0x73 || currentPosition[2] != 0x6E || currentPosition[3] != 0x81)
        {
            openknx.debug("FlashUserData", "   - No data found");
            return;
        }

        // read size
        currentPosition = (currentPosition - FLASH_USER_DATA_CHK_LEN - FLASH_USER_DATA_SIZE_LEN);
        userDataSize = (currentPosition[0] << 8) + currentPosition[1];

        // check
        currentPosition = (currentPosition - userDataSize);
        if (!verifyChecksum(currentPosition, userDataSize + FLASH_USER_DATA_SIZE_LEN + FLASH_USER_DATA_CHK_LEN))
        {
            openknx.debug("FlashUserData", "   - Checksum invalid!");
            return;
        }

        // printHEX("DATA: ", currentPosition, userDataSize);
        while (userDataProcessed < userDataSize)
        {
            moduleId = currentPosition[0];
            moduleSize = (currentPosition[1] << 8) + currentPosition[2];
            currentPosition = (currentPosition + FLASH_USER_DATA_MODULE_ID_LEN + FLASH_USER_DATA_SIZE_LEN);
            userDataProcessed += FLASH_USER_DATA_MODULE_ID_LEN + FLASH_USER_DATA_SIZE_LEN + moduleSize;
            module = openknx.getModule(moduleId);
            if (module == nullptr)
            {
                openknx.debug("FlashUserData", "  skip module with id %i (not found)", moduleId);
                currentPosition = (currentPosition + moduleSize);
                continue;
            }
            else
            {
                openknx.debug("FlashUserData", "  restore module %s with %i bytes", module->name(), moduleSize);
                module->restoreUserData(currentPosition, moduleSize);
                currentPosition = (currentPosition + moduleSize);
            }
        }
        openknx.debug("FlashUserData", "  complete (%i)", millis() - start);
    }

    void FlashUserData::save(bool force /* = false */)
    {
        uint32_t start = millis();
        bool moduleSaveStatus = false;
        uint8_t checksum = 0;
        uint8_t moduleId = 0;
        uint16_t userDataSize = 0;
        uint16_t moduleUserDataSize = 0;
        uint16_t maxModuleSize = 0;
        uint32_t currentPosition = 0;

        // table is not loaded (ets prog running) and save is not possible
        if (!knx.configured())
            return;

        // we have to ensure, that save is not called too often, because flash memory
        // does not survive too many writes
        if (!force && lastWrite > 0 && !delayCheck(lastWrite, FLASH_USER_DATA_WRITE_LIMIT))
            return;

        openknx.debug("FlashUserData", "save <%i>", force);

        // determine some values
        Modules *modules = openknx.getModules();
        userDataSize = 0;
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            moduleUserDataSize = modules->list[i - 1]->userDataSize();
            maxModuleSize = MAX(maxModuleSize, moduleUserDataSize);

            userDataSize += moduleUserDataSize +
                            FLASH_USER_DATA_MODULE_ID_LEN +
                            FLASH_USER_DATA_SIZE_LEN;
        }
        openknx.debug("FlashUserData", "userDataSize: %i", userDataSize);
        uint8_t *buffer = new uint8_t[maxModuleSize];

        // start pointer
        currentPosition = flashSize -
                          userDataSize -
                          FLASH_USER_DATA_META_LEN;

        openknx.debug("FlashUserData", "startPosition: %i", currentPosition);
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            // clear buffer
            memset(buffer, 0, maxModuleSize);

            // get data
            moduleUserDataSize = modules->list[i - 1]->userDataSize();
            moduleSaveStatus = modules->list[i - 1]->saveUserData(buffer);
            moduleId = modules->ids[i - 1];

            // write data
            currentPosition = writeFlash(currentPosition, moduleId);
            checksum += moduleId;
            if (!moduleSaveStatus)
            {
                // status false add empty value
                currentPosition = writeFlash(currentPosition, (uint16_t)0);
                continue;
            }

            checksum += calcChecksum(moduleUserDataSize);
            currentPosition = writeFlash(currentPosition, moduleUserDataSize);
            checksum += calcChecksum(buffer, moduleUserDataSize);
            currentPosition = writeFlash(currentPosition, buffer, moduleUserDataSize);
        }

        // write userDataSize
        currentPosition = writeFlash(currentPosition, userDataSize);
        checksum += calcChecksum(userDataSize);

        // write checksum
        currentPosition = writeFlash(currentPosition, checksum);

        uint8_t magicWord[4] = {0xDA, 0x73, 0x6E, 0x81};
        currentPosition = writeFlash(currentPosition, magicWord, 4);

        knx.platform().commitNonVolatileMemory();
        printHEX("DATA: ", flashStart + flashSize - userDataSize - FLASH_USER_DATA_META_LEN, userDataSize + FLASH_USER_DATA_META_LEN);

        lastWrite = millis();
        openknx.debug("FlashUserData", "  complete (%i)", millis() - start);

        // delete userData;
    }

    uint32_t FlashUserData::writeFlash(uint32_t relativeAddress, uint8_t *buffer, size_t size)
    {
        return knx.platform().writeNonVolatileMemory(relativeAddress, buffer, size);
    }

    uint32_t FlashUserData::writeFlash(uint32_t relativeAddress, uint8_t data)
    {
        uint8_t *buffer = new uint8_t[1];
        buffer[0] = data;
        return writeFlash(relativeAddress, buffer);
    }

    uint32_t FlashUserData::writeFlash(uint32_t relativeAddress, uint16_t data)
    {
        uint8_t *buffer = new uint8_t[1];
        buffer[0] = data >> 8;
        buffer[1] = data & 0xff;
        return writeFlash(relativeAddress, buffer, 2);
    }

    uint16_t FlashUserData::calcUserDataSize()
    {
        uint16_t len = FLASH_USER_DATA_MAGICWORD_LEN +
                       FLASH_USER_DATA_SIZE_LEN +
                       FLASH_USER_DATA_CHK_LEN;

        Modules *modules = openknx.getModules();
        for (uint8_t i = 1; i <= modules->count; i++)
            len += modules->list[i - 1]->userDataSize() +
                   FLASH_USER_DATA_SIZE_LEN;

        return len;
    }

    uint8_t FlashUserData::calcChecksum(uint16_t data)
    {
        return (data >> 8) + (data & 0xff);
    }

    uint8_t FlashUserData::calcChecksum(uint8_t *data, uint16_t len)
    {
        uint8_t sum = 0;

        for (uint16_t i = 0; i < len; i++)
            sum = sum + data[i];

        return sum;
    }

    // write checksum behind data
    void FlashUserData::writeChecksum(uint8_t *data, uint16_t len)
    {
        data[len] = calcChecksum(data, len);
    }

    bool FlashUserData::verifyChecksum(uint8_t *data, uint16_t len)
    {
        return data[len - 1] == calcChecksum(data, len - 1);
    }
} // namespace OpenKNX