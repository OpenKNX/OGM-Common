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
        openknx.debug("FlashUserData", "Load data");
        uint32_t start = millis();
        uint16_t currentPosition = 0;
        uint8_t *positionMagicword;
        uint8_t *positionUserDataSize;
        uint8_t *positionUserData;
        uint8_t *currentUserData;
        uint16_t moduleSize = 0;
        uint16_t userDataLen = 0;

        // check magicwords exists
        positionMagicword = flashStart + flashSize - FLASH_USER_DATA_MAGICWORD_LEN;
        if (positionMagicword[0] != 0xDA || positionMagicword[1] != 0x77 || positionMagicword[2] != 0x6E || positionMagicword[3] != 0x82)
        {
            openknx.debug("FlashUserData", "   - No data found");
            return;
        }

        // read size
        positionUserDataSize = (positionMagicword - FLASH_USER_DATA_CHK_LEN - FLASH_USER_DATA_SIZE_LEN);
        userDataLen = (positionUserDataSize[0] << 8) + positionUserDataSize[1];

        // check
        positionUserData = (positionUserDataSize - userDataLen);
        if (!verifyChecksum(positionUserData, userDataLen + FLASH_USER_DATA_SIZE_LEN + FLASH_USER_DATA_CHK_LEN))
        {
            openknx.debug("FlashUserData", "   - Checksum invalid!");
            return;
        }

        Modules *modules = openknx.getModules();
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            Module *module = modules->list[i - 1];
            currentUserData = (positionUserDataSize - currentPosition - FLASH_USER_DATA_SIZE_LEN);
            moduleSize = (currentUserData[0] << 8) + currentUserData[1];
            currentPosition += moduleSize + FLASH_USER_DATA_SIZE_LEN;
            currentUserData = (positionUserDataSize - currentPosition);
            openknx.debug("FlashUserData", "  %s (%i bytes)", module->name(), moduleSize);
            module->restoreUserData(currentUserData, moduleSize);
        }

        openknx.debug("FlashUserData", "  complete (%i)", millis() - start);
    }

    void FlashUserData::save(bool force /* = false */)
    {
        uint32_t start = millis();
        uint16_t userDataSize = calcUserDataSize();

        // skip if not userdata available
        if (userDataSize == 0)
            return;

        // table is not loaded (ets prog running) and save is not possible
        if (!knx.configured())
            return;

        // we have to ensure, that save is not called too often, because flash memory
        // does not survive too many writes
        if (!force && lastWrite > 0 && !delayCheck(lastWrite, FLASH_USER_DATA_WRITE_LIMIT))
            return;

        openknx.debug("FlashUserData", "save <%i>", force);

        uint8_t *userData = new uint8_t[userDataSize];
        memset(userData, 0, userDataSize);

        uint16_t currentLen = 0;
        uint16_t currentPosition = 0;
        uint8_t *currentUserData;
        uint16_t moduleSize = 0;

        currentPosition += FLASH_USER_DATA_MAGICWORD_LEN + FLASH_USER_DATA_SIZE_LEN + FLASH_USER_DATA_CHK_LEN;

        Modules *modules = openknx.getModules();
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            // module user data size
            moduleSize = modules->list[i - 1]->userDataSize();

            // move pointer to correct position
            currentPosition += moduleSize + FLASH_USER_DATA_SIZE_LEN;
            currentUserData = (userData + userDataSize - currentPosition);

            // fill module user data to pointer
            modules->list[i - 1]->saveUserData(currentUserData);

            // save module size after user data
            currentUserData[moduleSize] = moduleSize >> 8;
            currentUserData[moduleSize + 1] = moduleSize & 0xff;

            // store sum leng
            currentLen += moduleSize + FLASH_USER_DATA_SIZE_LEN;
        }

        currentUserData = (userData + userDataSize - FLASH_USER_DATA_SIZE_LEN - FLASH_USER_DATA_CHK_LEN - FLASH_USER_DATA_MAGICWORD_LEN);
        currentUserData[0] = currentLen >> 8;
        currentUserData[1] = currentLen & 0xff;

        // checksum
        writeChecksum(userData, currentLen + FLASH_USER_DATA_SIZE_LEN);

        // write magicword
        currentUserData = (userData + userDataSize - FLASH_USER_DATA_MAGICWORD_LEN);
        currentUserData[0] = 0xDA;
        currentUserData[1] = 0x77;
        currentUserData[2] = 0x6E;
        currentUserData[3] = 0x82;

        // write to flash
        // printHEX("DATA: ", userData, userDataSize);
        // openknx.debug("FlashUserData", "  len (%i)", userDataSize);
        // knx.platform().writeNonVolatileMemory(flashSize - userDataSize, userData, userDataSize);
        // knx.platform().commitNonVolatileMemory();

        lastWrite = millis();
        openknx.debug("FlashUserData", "  complete (%i)", millis() - start);

        delete userData;
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