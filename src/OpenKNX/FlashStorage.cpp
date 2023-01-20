#include "OpenKNX/FlashStorage.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    FlashStorage::FlashStorage()
    {
        _flashStart = knx.platform().getNonVolatileMemoryStart();
        _flashSize = knx.platform().getNonVolatileMemorySize();
    }

    FlashStorage::~FlashStorage()
    {}

    void FlashStorage::load()
    {
        openknx.paramTimer(1, 1);
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
        uint16_t dataSize = 0;
        uint16_t dataProcessed = 0;
        Module *module = nullptr;

        // check magicwords exists
        currentPosition = _flashStart + _flashSize - FLASH_DATA_META_LEN;
        if (FLASH_DATA_INIT != getInt(currentPosition + FLASH_DATA_META_LEN - FLASH_DATA_INIT_LEN))
        {
            openknx.debug("FlashStorage", "   - Abort: No data found");
            return;
        }

        // read size
        dataSize = (currentPosition[FLASH_DATA_META_LEN - 8] << 8) + currentPosition[FLASH_DATA_META_LEN - 7];

        // read FirmwareVersion
        _lastOpenKnxId = currentPosition[0];
        _lastApplicationNumber = currentPosition[1];
        openknx.debug("FlashStorage", "  ApplicationNumber: %i", _lastApplicationNumber);
        _lastApplicationVersion = getWord(currentPosition + 2);
        openknx.debug("FlashStorage", "  ApplicationVersion: %i", _lastApplicationVersion);

        // check
        currentPosition = (currentPosition - dataSize);
        if (!verifyChecksum(currentPosition, dataSize + FLASH_DATA_META_LEN - FLASH_DATA_INIT_LEN))
        {
            openknx.debug("FlashStorage", "   - Abort: Checksum invalid!");
            return;
        }

        // check apliicationNumber
        if (_lastOpenKnxId != openknx.openKnxId() || _lastApplicationNumber != openknx.applicationNumber())
        {
            openknx.debug("FlashStorage", "  - Abort: Data from other application");
            return;
        }

#ifdef FLASH_DATA_DEBUG
        openknx.debugHex("FlashStorage", currentPosition, dataSize + FLASH_DATA_META_LEN);
#endif

        while (dataProcessed < dataSize)
        {
            moduleId = currentPosition[0];
            moduleSize = getWord(currentPosition + 1);
            currentPosition = (currentPosition + FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN);
            dataProcessed += FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN + moduleSize;
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
                openknx.debugHex("FlashStorage", currentPosition, moduleSize);
#endif
            }
            currentPosition = (currentPosition + moduleSize);
        }
    }

    void FlashStorage::save(bool force /* = false */)
    {
        uint32_t start = millis();
        uint8_t moduleId = 0;
        uint16_t dataSize = 0;
        uint16_t moduleSize = 0;
        uint32_t currentPosition = 0;
        Module *module = nullptr;

        // table is not loaded (ets prog running) and save is not possible
        if (!knx.configured())
            return;

        // we have to ensure, that save is not called too often, because flash memory
        // does not survive too many writes
        if (!force && _lastWrite > 0 && !delayCheck(_lastWrite, FLASH_DATA_WRITE_LIMIT))
            return;

        openknx.debug("FlashStorage", "save <%i>", force);

        // determine some values
        Modules *modules = openknx.getModules();
        dataSize = 0;
        for (uint8_t i = 1; i <= modules->count; i++)
        {
            dataSize += modules->list[i - 1]->flashSize() +
                        FLASH_DATA_MODULE_ID_LEN +
                        FLASH_DATA_SIZE_LEN;
        }

#ifdef FLASH_DATA_DEBUG
        openknx.debug("FlashStorage", "  dataSize: %i", dataSize);
#endif

        // start point
        _currentWriteAddress = _flashSize -
                               dataSize -
                               FLASH_DATA_META_LEN;

#ifdef FLASH_DATA_DEBUG
        openknx.debug("FlashStorage", "  startPosition: %i", _currentWriteAddress);
#endif

        for (uint8_t i = 1; i <= modules->count; i++)
        {
            // get data
            module = modules->list[i - 1];
            moduleSize = module->flashSize();
            moduleId = modules->ids[i - 1];

            // write data
            _maxWriteAddress = _currentWriteAddress +
                               FLASH_DATA_MODULE_ID_LEN +
                               FLASH_DATA_SIZE_LEN;
            writeByte(moduleId);
            writeWord(moduleSize);

            _maxWriteAddress = _currentWriteAddress + moduleSize;

            openknx.debug("FlashStorage", "  save module %s (%i) wir %i bytes", module->name(), moduleId, moduleSize);
            module->writeFlash();
            zeroize();
        }

        // write magicword
        _maxWriteAddress = _currentWriteAddress + FLASH_DATA_META_LEN;

        // application info
        writeByte(openknx.openKnxId());
        writeByte(openknx.applicationNumber());
        writeWord(openknx.applicationVersion());

        // write size
        writeWord(dataSize);

        // write checksum
        writeWord(_checksum);

        // write init
        writeInt(FLASH_DATA_INIT);

        knx.platform().commitNonVolatileMemory();
#ifdef FLASH_DATA_DEBUG
        openknx.debugHex("FlashStorage", _flashStart + _flashSize - dataSize - FLASH_DATA_META_LEN, dataSize + FLASH_DATA_META_LEN);
#endif

        _lastWrite = millis();
        openknx.debug("FlashStorage", "  complete (%i)", _lastWrite - start);
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
        if ((_currentWriteAddress + size) > _maxWriteAddress)
        {
            openknx.debug("FlashStorage", "write not allow");
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
            openknx.debug("FlashStorage", "write not allow");
            return;
        }

        // for (uint16_t i = 0; i < size; i++)
        //     _checksum += value;

        for (uint16_t i = 0; i < size; i++)
            write(&value, 1);

        // _currentWriteAddress = knx.platform().writeNonVolatileMemory(_currentWriteAddress, value, size);
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
        uint16_t fillSize = (_maxWriteAddress - _currentWriteAddress);
        if (fillSize == 0)
            return;

#ifdef FLASH_DATA_DEBUG
        openknx.debug("FlashStorage", "    zeroize %i", fillSize);
#endif
        write((uint8_t)0xFF, fillSize);
    }

    uint16_t FlashStorage::applicationVersion()
    {
        return _lastApplicationVersion;
    }
} // namespace OpenKNX