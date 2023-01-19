#include "OpenKNX/FlashStorage.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    FlashStorage::FlashStorage()
    {
        flashStart = knx.platform().getNonVolatileMemoryStart();
        flashSize = knx.platform().getNonVolatileMemorySize();
    }

    FlashStorage::~FlashStorage()
    {}

    void FlashStorage::load()
    {
        uint32_t start = millis();
        bool *loadedModules = new bool[OPENKNX_MAX_MODULES];
        openknx.debug("FlashStorage", "load");
        readData(loadedModules);
        initUnloadedModules(loadedModules);
        openknx.debug("FlashStorage", "  complete (%i)", millis() - start);
    }

    void FlashStorage::initUnloadedModules(bool *loadedModules)
    {
        Modules *modules = openknx.getModules();
        Module *module = nullptr;
        uint8_t moduleId = 0;
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            module = modules->list[i - 1];
            moduleId = modules->ids[i - 1];
            if (!loadedModules[moduleId])
            {
                openknx.debug("FlashStorage", "  init module %s (%i)", module->name(), moduleId);
                module->readFlash(new uint8_t[0], 0);
            }
        }
    }

    void FlashStorage::readData(bool *loadedModules)
    {
        uint8_t *currentPosition;
        uint8_t moduleId = 0;
        uint16_t moduleSize = 0;
        uint16_t userDataSize = 0;
        uint16_t userDataProcessed = 0;
        Module *module = nullptr;

        // check magicwords exists
        currentPosition = flashStart + flashSize - FLASH_DATA_META_LEN;
        if (FLASH_DATA_INIT != getInt(currentPosition + FLASH_DATA_META_LEN - FLASH_DATA_INIT_LEN))
        {
            openknx.debug("FlashStorage", "   - Abort: No data found");
            return;
        }

        // read size
        userDataSize = (currentPosition[FLASH_DATA_META_LEN - 8] << 8) + currentPosition[FLASH_DATA_META_LEN - 7];

        // read FirmwareVersion
        lastOpenKnxId = currentPosition[0];
        lastApplicationNumber = currentPosition[1];
        openknx.debug("FlashStorage", "  ApplicationNumber: %i", lastApplicationNumber);
        lastApplicationVersion = getWord(currentPosition + 2);
        openknx.debug("FlashStorage", "  ApplicationVersion: %i", lastApplicationVersion);

        // check
        currentPosition = (currentPosition - userDataSize);
        if (!verifyChecksum(currentPosition, userDataSize + FLASH_DATA_META_LEN - FLASH_DATA_INIT_LEN))
        {
            openknx.debug("FlashStorage", "   - Abort: Checksum invalid!");
            return;
        }

        // check apliicationNumber
        if (lastOpenKnxId != openknx.openKnxId() || lastApplicationNumber != openknx.applicationNumber())
        {
            openknx.debug("FlashStorage", "  - Abort: Data from other application");
            return;
        }

#ifdef FLASH_DATA_DEBUG
        printHEX("DATA: ", currentPosition, userDataSize + FLASH_DATA_META_LEN);
#endif

        while (userDataProcessed < userDataSize)
        {
            moduleId = currentPosition[0];
            moduleSize = getWord(currentPosition + 1);
            currentPosition = (currentPosition + FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN);
            userDataProcessed += FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN + moduleSize;
            module = openknx.getModule(moduleId);
            if (module == nullptr)
            {
                openknx.debug("FlashStorage", "  skip module with id %i (not found)", moduleId);
            }
            else
            {
                openknx.debug("FlashStorage", "  restore module %s (%i) with %i bytes", module->name(), moduleId, moduleSize);
                module->readFlash(currentPosition, moduleSize);
                loadedModules[moduleId] = true;
#ifdef FLASH_DATA_DEBUG
                printHEX("DATA: ", currentPosition, moduleSize);
#endif
            }
            currentPosition = (currentPosition + moduleSize);
        }
    }

    void FlashStorage::save(bool force /* = false */)
    {
        uint32_t start = millis();
        uint8_t moduleId = 0;
        uint16_t userDataSize = 0;
        uint16_t moduleUserDataSize = 0;
        uint32_t currentPosition = 0;
        Module *module = nullptr;

        // table is not loaded (ets prog running) and save is not possible
        if (!knx.configured())
            return;

        // we have to ensure, that save is not called too often, because flash memory
        // does not survive too many writes
        if (!force && lastWrite > 0 && !delayCheck(lastWrite, FLASH_DATA_WRITE_LIMIT))
            return;

        openknx.debug("FlashStorage", "save <%i>", force);

        // determine some values
        Modules *modules = openknx.getModules();
        userDataSize = 0;
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            userDataSize += modules->list[i - 1]->flashSize() +
                            FLASH_DATA_MODULE_ID_LEN +
                            FLASH_DATA_SIZE_LEN;
        }

#ifdef FLASH_DATA_DEBUG
        openknx.debug("FlashStorage", "  userDataSize: %i", userDataSize);
#endif

        // start point
        currentWriteAddress = flashSize -
                              userDataSize -
                              FLASH_DATA_META_LEN;

#ifdef FLASH_DATA_DEBUG
        openknx.debug("FlashStorage", "  startPosition: %i", currentWriteAddress);
#endif

        for (uint8_t i = 1; i <= modules->count; i++)
        {
            // get data
            module = modules->list[i - 1];
            moduleUserDataSize = module->flashSize();
            moduleId = modules->ids[i - 1];

            // write data
            maxWriteAddress = currentWriteAddress +
                              FLASH_DATA_MODULE_ID_LEN +
                              FLASH_DATA_SIZE_LEN;
            writeByte(moduleId);
            writeWord(moduleUserDataSize);

            maxWriteAddress = currentWriteAddress + moduleUserDataSize;

            openknx.debug("FlashStorage", "  save module %s (%i) wir %i bytes", module->name(), moduleId, moduleUserDataSize);
            module->writeFlash();
            zeroize();
        }

        // write magicword
        maxWriteAddress = currentWriteAddress + FLASH_DATA_META_LEN;

        // application info
        writeByte(openknx.openKnxId());
        writeByte(openknx.applicationNumber());
        writeWord(openknx.applicationVersion());

        // write size
        writeWord(userDataSize);

        // write checksum
        writeWord(checksum);

        // write init
        writeInt(FLASH_DATA_INIT);

        knx.platform().commitNonVolatileMemory();
#ifdef FLASH_DATA_DEBUG
        printHEX("DATA: ", flashStart + flashSize - userDataSize - FLASH_DATA_META_LEN, userDataSize + FLASH_DATA_META_LEN);
#endif

        lastWrite = millis();
        openknx.debug("FlashStorage", "  complete (%i)", lastWrite - start);
    }

    uint16_t FlashStorage::calcChecksum(uint16_t data)
    {
        return (data >> 8) + (data & 0xff);
    }

    uint16_t FlashStorage::calcChecksum(uint8_t *data, uint16_t size)
    {
        uint16_t sum = 0;

        for (uint16_t i = 0; i < size; i++)
            sum = sum + data[i];

        return sum;
    }

    bool FlashStorage::verifyChecksum(uint8_t *data, uint16_t size)
    {
        return ((data[size - 2] << 8) + data[size - 1]) == calcChecksum(data, size - 2);
    }

    void FlashStorage::write(uint8_t *buffer, uint16_t size)
    {
        if ((currentWriteAddress + size) > maxWriteAddress)
        {
            openknx.debug("FlashStorage", "write not allow");
            return;
        }

        for (uint16_t i = 0; i < size; i++)
            checksum += buffer[i];

        currentWriteAddress = knx.platform().writeNonVolatileMemory(currentWriteAddress, buffer, size);
    }

    void FlashStorage::writeByte(uint8_t value)
    {
        uint8_t *buffer = new uint8_t[1];
        buffer[0] = value;
        write(buffer);
        delete buffer;
    }

    void FlashStorage::writeWord(uint16_t value)
    {
        uint8_t *buffer = new uint8_t[2];
        buffer[0] = ((value >> 8) & 0xff);
        buffer[1] = (value & 0xff);
        write(buffer, 2);
        delete buffer;
    }

    void FlashStorage::writeInt(uint32_t value)
    {
        uint8_t *buffer = new uint8_t[4];
        buffer[0] = ((value >> 24) & 0xff);
        buffer[1] = ((value >> 16) & 0xff);
        buffer[2] = ((value >> 8) & 0xff);
        buffer[3] = (value & 0xff);
        write(buffer, 4);
        delete buffer;
    }

    void FlashStorage::zeroize()
    {
        uint16_t fillSize = (maxWriteAddress - currentWriteAddress);
        if (fillSize == 0)
            return;

        uint8_t *buffer = new uint8_t[fillSize];
        memset(buffer, 0, fillSize);
        write(buffer, fillSize);
#ifdef FLASH_DATA_DEBUG
        openknx.debug("FlashStorage", "    zeroize %i", fillSize);
#endif
        delete buffer;
    }

    uint16_t FlashStorage::applicationVersion()
    {
        return lastApplicationVersion;
    }
} // namespace OpenKNX