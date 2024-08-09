#include "OpenKNX/Flash/Default.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Flash
    {
        uint8_t Default::nextVersion()
        {
#ifdef ARDUINO_ARCH_RP2040
            return slotVersion(_activeSlot) + 1;
#else
            return 0xFF;
#endif
        }

        std::string Default::logPrefix()
        {
            return "Flash<Default>";
        }

        void Default::load()
        {
            const uint32_t start = millis();
            loadedModules = new bool[openknx.modules.count];
            logInfoP("Load data from flash");
            logIndentUp();
            bool found = false;
            const bool slotValidA = validateSlot(false);
            if (slotValidA)
                found = true;

#ifdef ARDUINO_ARCH_RP2040
            const bool slotValidB = validateSlot(true);
            const uint8_t slotVersionA = slotVersion(false);
            const uint8_t slotVersionB = slotVersion(true);
            if (slotValidB)
            {
                found = true;

                if (
                    // when slot b valid und slot a not
                    !slotValidA ||
                    // when slot b newer than slot a
                    (
                        (slotVersionB != 0xFF && slotVersionA < slotVersionB) ||
                        (slotVersionA == 0xFF && slotVersionB == 0x0)))
                {
                    _activeSlot = true;
                }
            }
#endif

            if (!found)
            {
                logInfoP("Abort: No valid data found");
                logIndentDown();
                return;
            }

            loadModuleData();
            initUnloadedModules();

            // erase next slot
            eraseSlot(nextSlot());

            logInfoP("Loading completed (%ims)", millis() - start);
            logIndentDown();
        }

        uint16_t Default::slotOffset(bool slot)
        {
#ifdef ARDUINO_ARCH_RP2040
            return slotSize() + (slot ? slotSize() : 0);
#else
            return slotSize();
#endif
        }

        uint16_t Default::slotSize()
        {
#ifdef ARDUINO_ARCH_RP2040
            return openknx.openknxFlash.size() / 2;
#else
            return openknx.openknxFlash.size();
#endif
        }

        uint32_t Default::readOffset()
        {
            return slotOffset(_activeSlot);
        }

        uint32_t Default::writeOffset()
        {
            return slotOffset(nextSlot());
        }

        bool Default::nextSlot()
        {
#ifdef ARDUINO_ARCH_RP2040
            return !_activeSlot;
#else
            return false;
#endif
        }

        bool Default::validateSlot(bool slot)
        {

#ifndef ARDUINO_ARCH_RP2040
            if (slot)
                return false;
#endif
            logDebugP("Validate slot %i", slot);
            logIndentUp();
            logHexTraceP(openknx.openknxFlash.flashAddress() + slotOffset(slot) - FLASH_DATA_META_LEN, FLASH_DATA_META_LEN);

            // validate magicwords exists (at last position)
            _currentReadAddress = slotOffset(slot) - FLASH_DATA_INIT_LEN;
            if (FLASH_DATA_INIT != readInt())
            {
                logDebugP("No data found");
                logIndentDown();
                return false;
            }

            // validate FirmwareVersion/Number
            _currentReadAddress = slotOffset(slot) - FLASH_DATA_META_LEN;
            _lastFirmwareNumber = readWord();
            logDebugP("Firmware number: 0x%04X", _lastFirmwareNumber);
            _lastFirmwareVersion = readWord();
            logDebugP("Firmware version: %i", _lastFirmwareVersion);

            // validate checksum
            if (_lastFirmwareNumber != openknx.info.firmwareNumber())
            {
                logErrorP("Data from other application");
                logIndentDown();
                return false;
            }

            const uint16_t dataSize = readWord();
            logDebugP("Data size: %i", dataSize);

            const uint8_t version = readByte();
            // (version >= 0); // do nothing prevents warning for line above
            (void)version;

            logDebugP("Version: %i", version);

            const uint16_t checksum = readWord();
            logDebugP("Checksum: %i", checksum);

            // validate checksum
            _currentReadAddress = slotOffset(slot) - FLASH_DATA_META_LEN - dataSize;
            const uint16_t checksumSize = dataSize + FLASH_DATA_APP_LEN + FLASH_DATA_SIZE_LEN + FLASH_DATA_VERSION;
            if (!verifyChecksum(currentFlash(), checksumSize, checksum))
            {
                logErrorP("Checksum invalid!");
                logHexErrorP(openknx.openknxFlash.flashAddress() + slotOffset(slot) - FLASH_DATA_META_LEN - dataSize, checksumSize);
                logIndentDown();
                return false;
            }

            logIndentDown();
            return true;
        }

        uint8_t Default::slotVersion(bool slot)
        {
#ifndef ARDUINO_ARCH_RP2040
            if (slot)
                return false;
#endif
            _currentReadAddress = slotOffset(slot) - FLASH_DATA_INIT_LEN - FLASH_DATA_CHK_LEN - FLASH_DATA_VERSION;
            return readByte();
        }

        /**
         * Initialize all modules expecting data in flash, but not loaded yet.
         */
        void Default::initUnloadedModules()
        {
            for (uint8_t i = 0; i < openknx.modules.count; i++)
            {
                // check module expectation and load state
                Module *module = openknx.modules.list[i];
                const uint8_t moduleId = openknx.modules.ids[i];
                const uint16_t moduleSize = module->flashSize();

                if (moduleSize > 0 && !loadedModules[moduleId])
                {
                    logDebugP("Init unloaded module %s (%i)", module->name().c_str(), moduleId);
                    module->readFlash(new uint8_t[0], 0);
                }
            }
        }

        void Default::eraseSlot(bool slot)
        {
#ifdef ARDUINO_ARCH_RP2040
            // On RP2020 we need to erase next slot for fast writing on powerloss
    #ifdef OPENKNX_DEBUG
            const uint32_t start = millis();
    #endif
            logDebugP("Erase slot %i", slot);
            logIndentUp();
            openknx.openknxFlash.write(slotOffset(slot) - slotSize(), 0xFF, slotSize());
            openknx.openknxFlash.commit();
            logDebugP("Erase completed (%ims)", millis() - start);
            logIndentDown();
#endif
        }

        void Default::loadModuleData()
        {
            logInfoP("Load module data (from slot %i)", _activeSlot);
            logIndentUp();

            // reread data size for calc
            _currentReadAddress = readOffset() - FLASH_DATA_INIT_LEN - FLASH_DATA_CHK_LEN - FLASH_DATA_VERSION - FLASH_DATA_SIZE_LEN;
            const uint16_t dataSize = readWord();

            // process data
            _currentReadAddress = readOffset() - FLASH_DATA_META_LEN - dataSize;

            uint32_t dataProcessed = 0;
            while (dataProcessed < dataSize)
            {
                uint8_t moduleId = readByte();
                uint16_t moduleSize = readWord();
                Module *module = openknx.getModule(moduleId);
                dataProcessed += FLASH_DATA_MODULE_ID_LEN + FLASH_DATA_SIZE_LEN + moduleSize;
                if (module == nullptr)
                {
                    logInfoP("Skip module with id %i (not found)", moduleId);
                }
                else
                {
                    logInfoP("Restore module %s (%i) with %i bytes", module->name().c_str(), moduleId, moduleSize);
                    logIndentUp();
                    logHexTraceP(currentFlash(), moduleSize);
                    module->readFlash(currentFlash(), moduleSize);
                    loadedModules[moduleId] = true;
                    logIndentDown();
                }
                _currentReadAddress = readOffset() - FLASH_DATA_META_LEN - dataSize + dataProcessed;
            }
            logIndentDown();
        }

        void Default::save(bool force /* = false */)
        {
            openknx.common.skipLooptimeWarning();

            _checksum = 0;
            uint32_t start = millis();

            // table is not loaded (ets prog running) and save is not possible
            if (!knx.configured())
                return;

            // we have to ensure, that save is not called too often, because flash memory
            // does not survive too many writes
            if (!force && _lastWrite > 0 && !delayCheck(_lastWrite, FLASH_DATA_WRITE_LIMIT))
                return;

            _lastWrite = millis();

            logBegin();
            logInfoP("Save data to flash%s", force ? " (force)" : "");
            logIndentUp();
            logDebugP("Slot %i", nextSlot());

            // determine some values
            uint16_t dataSize = 0;
            for (uint8_t i = 0; i < openknx.modules.count; i++)
            {
                const uint16_t moduleSize = openknx.modules.list[i]->flashSize();
                if (moduleSize == 0)
                    continue;

                dataSize += moduleSize +
                            FLASH_DATA_MODULE_ID_LEN +
                            FLASH_DATA_SIZE_LEN;
            }

            logTraceP("dataSize: %i", dataSize);

            // start point
            _currentWriteAddress = writeOffset() -
                                   dataSize -
                                   FLASH_DATA_META_LEN;

            logTraceP("startPosition: %i", _currentWriteAddress);

            for (uint8_t i = 0; i < openknx.modules.count; i++)
            {
                // get data
                Module *module = openknx.modules.list[i];
                uint16_t moduleSize = module->flashSize();
                uint8_t moduleId = openknx.modules.ids[i];

                if (moduleSize == 0)
                    continue;

                _maxWriteAddress = _currentWriteAddress +
                                   FLASH_DATA_MODULE_ID_LEN +
                                   FLASH_DATA_SIZE_LEN;

                // write header for module data
                writeByte(moduleId);
                writeWord(moduleSize);

                // write the module data
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
            logDebugP("Version %i", nextVersion());
            writeByte(nextVersion());

            // write checksum
            writeWord(_checksum);

            // block of metadata
            writeInt(FLASH_DATA_INIT);

            openknx.openknxFlash.commit();
            logHexTraceP(openknx.openknxFlash.flashAddress() + writeOffset() - dataSize - FLASH_DATA_META_LEN, dataSize + FLASH_DATA_META_LEN);

            logInfoP("Save completed (%ims)", millis() - start);

#ifdef ARDUINO_ARCH_RP2040
            // new active slot
            _activeSlot = !_activeSlot;

            // erase next slot
            eraseSlot(nextSlot());
#endif

            logIndentDown();
            logEnd();
        }

        uint8_t *Default::currentFlash()
        {
            return openknx.openknxFlash.flashAddress() + _currentReadAddress;
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

            _currentWriteAddress = openknx.openknxFlash.write(_currentWriteAddress, buffer, size);
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

            _currentWriteAddress = openknx.openknxFlash.write(_currentWriteAddress, value, size);
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

        void Default::writeFloat(float value)
        {
            write((uint8_t *)&value, 4);
        }

        void Default::writeLong(long value)
        {
            write((uint8_t *)&value, 8);
        }

        void Default::writeDouble(double value)
        {
            write((uint8_t *)&value, 8);
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
            return openknx.openknxFlash.readByte(_currentReadAddress - 1);
        }

        uint16_t Default::readWord()
        {
            _currentReadAddress += 2;
            return openknx.openknxFlash.readWord(_currentReadAddress - 2);
        }

        uint32_t Default::readInt()
        {
            _currentReadAddress += 4;
            return openknx.openknxFlash.readInt(_currentReadAddress - 4);
        }

        float Default::readFloat()
        {
            _currentReadAddress += 4;
            return openknx.openknxFlash.readFloat(_currentReadAddress - 4);
        }

        long Default::readLong()
        {
            _currentReadAddress += 8;
            return openknx.openknxFlash.readLong(_currentReadAddress - 8);
        }

        double Default::readDouble()
        {
            _currentReadAddress += 8;
            return openknx.openknxFlash.readDouble(_currentReadAddress - 8);
        }

        uint16_t Default::firmwareVersion()
        {
            return _lastFirmwareVersion;
        }

        uint32_t Default::lastWrite()
        {
            return _lastWrite;
        }
    } // namespace Flash
} // namespace OpenKNX