#include "OpenKNX/Flash/Default.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    namespace Flash
    {
        Default::Default()
        {
#ifdef ARDUINO_ARCH_SAMD
            _flash = new OpenKNX::Flash::Samd(OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE, "Default");
#else
            _flash = new OpenKNX::Flash::RP2040(OPENKNX_FLASH_OFFSET, OPENKNX_FLASH_SIZE, "Default");
#endif
        }

        void Default::init()
        {
        }

        std::string Default::logPrefix()
        {
            return "Flash<Default>";
        }

        void Default::load()
        {
            uint32_t start = millis();
            loadedModules = new bool[openknx.getModules()->count];
            logInfoP("Load data from flash");
            logIndentUp();
            readData();
            initUnloadedModules();
            logInfoP("Loading completed (%ims)", millis() - start);
            logIndentDown();
        }

        /**
         * Initialize all modules expecting data in flash, but not loaded yet.
         */
        void Default::initUnloadedModules()
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
                    logDebugP("Init module %s (%i)", module->name().c_str(), moduleId);
                    module->readFlash(new uint8_t[0], 0);
                }
            }
        }

        /**
         * Check version/format/checksum and let matching modules read their data from flash-Flash
         */
        void Default::readData()
        {
            // check magicwords exists
            _currentReadAddress = _flash->size() - FLASH_DATA_INIT_LEN;
            if (FLASH_DATA_INIT != readInt())
            {
                logInfoP("Abort: No data found");
                return;
            }

            // APP
            _currentReadAddress = _flash->size() - FLASH_DATA_META_LEN;
            _lastFirmwareNumber = readWord();
            logDebugP("FirmwareNumber: 0x%04X", _lastFirmwareNumber);
            _lastFirmwareVersion = readWord();
            logDebugP("FirmwareVersion: %i", _lastFirmwareVersion);

            // check FirmwareNumber
            if (_lastFirmwareNumber != openknx.info.firmwareNumber())
            {
                logErrorP("Abort: Data from other application");
                return;
            }

            // SIZE
            uint16_t dataSize = readWord();
            logDebugP("DataSize: %i", dataSize);

            // VERSION
            uint8_t version = readByte();
            logDebugP("Write Version: %i", version);

            // Checksum
            uint16_t checksum = readWord();
            logDebugP("Checksum: %i", checksum);

            // validate checksum
            _currentReadAddress = _flash->size() - FLASH_DATA_META_LEN - dataSize;
            if (!verifyChecksum(currentFlash(), dataSize + FLASH_DATA_APP_LEN + FLASH_DATA_SIZE_LEN + FLASH_DATA_VERSION, checksum))
            {
                logErrorP("Abort: Checksum invalid!");
                logHexErrorP(currentFlash(), dataSize + FLASH_DATA_APP_LEN + FLASH_DATA_SIZE_LEN + FLASH_DATA_VERSION);
                return;
            }

            logHexTraceP(currentFlash(), dataSize + FLASH_DATA_META_LEN);

            uint32_t dataProcessed = 0;
            while (dataProcessed < dataSize)
            {
                uint8_t moduleId = readByte();
                uint16_t moduleSize = readWord();
                dataProcessed += FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN + moduleSize;
                Module *module = openknx.getModule(moduleId);
                if (module == nullptr)
                {
                    logDebugP("Skip module with id %i (not found)", moduleId);
                }
                else
                {
                    logDebugP("Restore module %s (%i) with %i bytes", module->name().c_str(), moduleId, moduleSize);
                    logIndentUp();
                    logHexTraceP(currentFlash(), moduleSize);
                    module->readFlash(currentFlash(), moduleSize);
                    loadedModules[moduleId] = true;
                    logIndentDown();
                }
                _currentReadAddress = (_currentReadAddress + moduleSize);
            }
        }

        void Default::save(bool force /* = false */)
        {
            _checksum = 0;
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
            logIndentUp();

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

            logTraceP("dataSize: %i", dataSize);

            // start point
            _currentWriteAddress = _flash->size() -
                                   dataSize -
                                   FLASH_DATA_META_LEN;

            logTraceP("startPosition: %i", _currentWriteAddress);

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

                logDebugP("Save module %s (%i) with %i bytes", module->name().c_str(), moduleId, moduleSize);
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

            // write version
            writeByte(0xFF);

            // write checksum
            writeWord(_checksum);

            // write init
            writeInt(FLASH_DATA_INIT);

            _flash->commit();
            logHexTraceP(_flash->flash() + _flash->size() - dataSize - FLASH_DATA_META_LEN, dataSize + FLASH_DATA_META_LEN);

            _lastWrite = millis();
            logInfoP("Save completed (%ims)", _lastWrite - start);
            logIndentDown();
        }

        uint8_t *Default::currentFlash()
        {
            return _flash->flash() + _currentReadAddress;
        }

        uint16_t Default::calcChecksum(uint8_t *data, uint16_t size)
        {
            uint16_t sum = 0;

            for (uint16_t i = 0; i < size; i++)
                sum = sum + data[i];

            return sum;
        }

        bool Default::verifyChecksum(uint8_t *data, uint16_t size, uint16_t checksum)
        {
            // logInfoP("verifyChecksum %i == %i", checksum, calcChecksum(data, size));
            return checksum == calcChecksum(data, size);
        }

        void Default::write(uint8_t *buffer, uint16_t size)
        {
            if ((_currentWriteAddress + size) > _maxWriteAddress)
            {
                logErrorP("    A module has tried to write more than it was allowed to write");
                return;
            }

            for (uint16_t i = 0; i < size; i++)
                _checksum += buffer[i];

            _currentWriteAddress = _flash->write(_currentWriteAddress, buffer, size);
        }

        void Default::write(uint8_t value, uint16_t size)
        {
            if ((_currentWriteAddress + size) > _maxWriteAddress)
            {
                logErrorP("    A module has tried to write more than it was allowed to write");
                return;
            }

            for (uint16_t i = 0; i < size; i++)
                _checksum += value;

            _currentWriteAddress = _flash->write(_currentWriteAddress, value, size);
        }

        void Default::writeByte(uint8_t value)
        {
            write((uint8_t *)&value);
        }

        void Default::writeWord(uint16_t value)
        {
            write((uint8_t *)&value, 2);
        }

        void Default::writeInt(uint32_t value)
        {
            write((uint8_t *)&value, 4);
        }

        void Default::writeFilldata()
        {
            uint16_t fillSize = (_maxWriteAddress - _currentWriteAddress);
            if (fillSize == 0)
                return;

            logTraceP("    writeFilldata %i", fillSize);
            write((uint8_t)FLASH_DATA_FILLBYTE, fillSize);
        }

        uint8_t *Default::read(uint16_t size /* = 1 */)
        {
            _currentReadAddress += size;
            return currentFlash() - size;
        }

        uint8_t Default::readByte()
        {
            _currentReadAddress += 1;
            return _flash->readByte(_currentReadAddress - 1);
        }

        uint16_t Default::readWord()
        {
            _currentReadAddress += 2;
            return _flash->readWord(_currentReadAddress - 2);
        }

        uint32_t Default::readInt()
        {
            _currentReadAddress += 4;
            return _flash->readInt(_currentReadAddress - 4);
        }

        uint16_t Default::firmwareVersion()
        {
            return _lastFirmwareVersion;
        }
    } // namespace Flash
} // namespace OpenKNX