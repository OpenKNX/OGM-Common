#include "OpenKNX/FlashStorage.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    std::string FlashStorage::logPrefix() {
        return "FlashStorage";
    }

    void FlashStorage::load()
    {
        _flashSize = knx.platform().getNonVolatileMemorySize();
        _flashStart = knx.platform().getNonVolatileMemoryStart();
        uint32_t start = millis();
        loadedModules = new bool[openknx.getModules()->count];
        logInfoP("Load data from flash");
        readData();
        initUnloadedModules();
        logInfoP("  Loading completed (%ims)", millis() - start);
    }

    /**
     * Initialize all modules expecting data in flash, but not loaded yet.
    */
    void FlashStorage::initUnloadedModules()
    {
        Modules *modules = openknx.getModules();
        Module *module = nullptr;
        uint8_t moduleId = 0;
        uint16_t moduleSize = 0;
        for (uint8_t i = 0; i < modules->count; i++)
        {
            // check module expectation and load state
            module = modules->list[i];
            moduleId = modules->ids[i];
            moduleSize = module->flashSize();

            if (moduleSize > 0 && !loadedModules[moduleId])
            {
                logDebugP("  Init module %s (%i)", module->name().c_str(), moduleId);
                module->readFlash(new uint8_t[0], 0);
            }
        }
    }

    /**
     * Check version/format/checksum and let matching modules read their data from flash-storage
    */
    void FlashStorage::readData()
    {
        uint8_t *currentPosition;
        uint8_t moduleId = 0;
        uint16_t moduleSize = 0;
        uint16_t dataSize = 0;
        uint16_t dataProcessed = 0;
        Module *module = nullptr;

        // check magicwords exists
        currentPosition = _flashStart + _flashSize - FLASH_DATA_META_LEN;
        if (FLASH_DATA_INIT != getInt(currentPosition + FLASH_DATA_META_LEN - FLASH_DATA_INIT_LEN))
        {
            logInfoP("   - Abort: No data found");
            return;
        }

        // read size
        dataSize = (currentPosition[FLASH_DATA_META_LEN - 8] << 8) + currentPosition[FLASH_DATA_META_LEN - 7];

        // read FirmwareVersion
        _currentReadAddress = currentPosition;
        _lastFirmwareNumber = readWord();
        logDebugP("  FirmwareNumber: 0x%04X", _lastFirmwareNumber);

        _lastFirmwareVersion = readWord();
        logDebugP("  FirmwareVersion: %i", _lastFirmwareVersion);

        // validate checksum
        currentPosition = (currentPosition - dataSize);
        if (!verifyChecksum(currentPosition, dataSize + FLASH_DATA_META_LEN - FLASH_DATA_INIT_LEN))
        {
            logErrorP("   - Abort: Checksum invalid!");
            logHexErrorP(currentPosition, dataSize + FLASH_DATA_META_LEN - FLASH_DATA_INIT_LEN);
            return;
        }

        // check FirmwareNumber
        if (_lastFirmwareNumber != openknx.info.firmwareNumber())
        {
            logErrorP("  - Abort: Data from other application");
            return;
        }

        logHexTraceP(currentPosition, dataSize + FLASH_DATA_META_LEN);

        while (dataProcessed < dataSize)
        {
            _currentReadAddress = currentPosition;
            moduleId = readByte();
            moduleSize = readWord();
            currentPosition = (currentPosition + FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN);
            dataProcessed += FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN + moduleSize;
            module = openknx.getModule(moduleId);
            if (module == nullptr)
            {
                logDebugP("  Skip module with id %i (not found)", moduleId);
            }
            else
            {
                logDebugP("  Restore module %s (%i) with %i bytes", module->name().c_str(), moduleId, moduleSize);
                _currentReadAddress = currentPosition;
                logHexTraceP(currentPosition, moduleSize);
                module->readFlash(currentPosition, moduleSize);
                loadedModules[moduleId] = true;
            }
            currentPosition = (currentPosition + moduleSize);
        }
    }

    void FlashStorage::save(bool force /* = false */)
    {
        _checksum = 0;
        _flashSize = knx.platform().getNonVolatileMemorySize();
        _flashStart = knx.platform().getNonVolatileMemoryStart();

        uint32_t start = millis();
        uint8_t moduleId = 0;
        uint16_t dataSize = 0;
        uint16_t moduleSize = 0;
        Module *module = nullptr;

        // table is not loaded (ets prog running) and save is not possible
        if (!knx.configured())
            return;

        // we have to ensure, that save is not called too often, because flash memory
        // does not survive too many writes
        if (!force && _lastWrite > 0 && !delayCheck(_lastWrite, FLASH_DATA_WRITE_LIMIT))
            return;

        logInfoP("Save data to flash%s", force ? " (force)" : "");

        // determine some values
        Modules *modules = openknx.getModules();
        dataSize = 0;
        for (uint8_t i = 0; i < modules->count; i++)
        {
            moduleSize = modules->list[i]->flashSize();
            if (moduleSize == 0)
                continue;

            dataSize += moduleSize +
                        FLASH_DATA_MODULE_ID_LEN +
                        FLASH_DATA_SIZE_LEN;
        }

        logTraceP("  dataSize: %i", dataSize);

        // start point
        _currentWriteAddress = _flashSize -
                               dataSize -
                               FLASH_DATA_META_LEN;

        logTraceP("  startPosition: %i", _currentWriteAddress);

        for (uint8_t i = 0; i < modules->count; i++)
        {
            // get data
            module = modules->list[i];
            moduleSize = module->flashSize();
            moduleId = modules->ids[i];

            if (moduleSize == 0)
                continue;

            // write data
            _maxWriteAddress = _currentWriteAddress +
                               FLASH_DATA_MODULE_ID_LEN +
                               FLASH_DATA_SIZE_LEN;

            writeByte(moduleId);

            writeWord(moduleSize);

            _maxWriteAddress = _currentWriteAddress + moduleSize;

            logDebugP("  Save module %s (%i) with %i bytes", module->name().c_str(), moduleId, moduleSize);
            module->writeFlash();
            writeFilldata();
        }

        // write magicword
        _maxWriteAddress = _currentWriteAddress + FLASH_DATA_META_LEN;

        // application info
        writeWord(openknx.info.firmwareNumber());
        writeWord(openknx.info.firmwareVersion());

        // write size
        writeWord(dataSize);

        // write checksum
        writeWord(_checksum);

        // write init
        writeInt(FLASH_DATA_INIT);

        knx.platform().commitNonVolatileMemory();
        logHexTraceP(_flashStart + _flashSize - dataSize - FLASH_DATA_META_LEN, dataSize + FLASH_DATA_META_LEN);

        _lastWrite = millis();
        logInfoP("  Save completed (%ims)", _lastWrite - start);
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
        // logInfoP("verifyChecksum %i == %i", ((data[size - 2] << 8) + data[size - 1]), calcChecksum(data, size - 2));
        return ((data[size - 2] << 8) + data[size - 1]) == calcChecksum(data, size - 2);
    }

    void FlashStorage::write(uint8_t *buffer, uint16_t size)
    {
        if ((_currentWriteAddress + size) > _maxWriteAddress)
        {
            logErrorP("    A module has tried to write more than it was allowed to write");
            return;
        }

        for (uint16_t i = 0; i < size; i++)
            _checksum += buffer[i];

        _currentWriteAddress = knx.platform().writeNonVolatileMemory(_currentWriteAddress, buffer, size);
    }

    void FlashStorage::write(uint8_t value, uint16_t size)
    {
        if ((_currentWriteAddress + size) > _maxWriteAddress)
        {
            logErrorP("    A module has tried to write more than it was allowed to write");
            return;
        }

        for (uint16_t i = 0; i < size; i++)
            _checksum += value;

        _currentWriteAddress = knx.platform().writeNonVolatileMemory(_currentWriteAddress, value, size);
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

    void FlashStorage::writeFilldata()
    {
        uint16_t fillSize = (_maxWriteAddress - _currentWriteAddress);
        if (fillSize == 0)
            return;

        logTraceP("    writeFilldata %i", fillSize);
        write((uint8_t)FLASH_DATA_FILLBYTE, fillSize);
    }

    uint8_t *FlashStorage::read(uint16_t size /* = 1 */)
    {
        uint8_t *address = _currentReadAddress;
        _currentReadAddress += size;
        return address;
    }

    uint8_t FlashStorage::readByte()
    {
        return read(1)[0];
    }

    uint16_t FlashStorage::readWord()
    {
        return (readByte() << 8) + readByte();
    }

    uint32_t FlashStorage::readInt()
    {
        return (readByte() << 24) + (readByte() << 16) + (readByte() << 8) + readByte();
    }

    uint16_t FlashStorage::firmwareVersion()
    {
        return _lastFirmwareVersion;
    }
} // namespace OpenKNX