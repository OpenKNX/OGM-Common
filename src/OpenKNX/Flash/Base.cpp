#include "OpenKNX/Flash/Base.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    namespace Flash
    {
        std::string Base::logPrefix()
        {
            return openknx.logger.logPrefix("Flash", _id);
        }

        void Base::printBaseInfo()
        {
            logInfoP("initalize %i bytes at 0x%X", _size, _offset);
            logIndentUp();
            logDebugP("sectorSize: %i", _sectorSize);
            logDebugP("startFree: %i", _startFree);
            logDebugP("endFree: %i", _endFree);
            logIndentDown();
        }

        void Base::validateParameters()
        {
            if (_size % _sectorSize)
                fatalError(1, "Flash: Size unaligned");
            if (_offset % _sectorSize)
                fatalError(1, "Flash: Offset unaligned");
            if (_size > _endFree)
                fatalError(1, "Flash: End behind free flash");
            if (_offset < _startFree) {
                logInfoP("%i < %i", _offset, _startFree);
                fatalError(1, "Flash: Offset start before free flash begin");

            }
        }

        uint32_t Base::sectorSize()
        {
            return _sectorSize;
        }

        uint32_t Base::startFree()
        {
            return _startFree;
        }

        uint32_t Base::endFree()
        {
            return _endFree;
        }

        uint32_t Base::size()
        {
            return _size;
        }

        uint32_t Base::startOffset()
        {
            return _offset;
        }

        uint16_t Base::sectorOfRelativeAddress(uint32_t relativeAddress)
        {
            return relativeAddress / _sectorSize;
        }

        bool Base::needEraseSector(uint16_t sector)
        {
            for (size_t i = 0; i < _sectorSize; i++)
                if ((flash() + sector * _sectorSize)[i] != 0xFF)
                    return true;

            return false;
        }

        bool Base::needWriteSector()
        {
            return memcmp(_buffer, flash() + _bufferSector * _sectorSize, _sectorSize);
        }

        bool Base::needEraseForBuffer()
        {
            uint8_t flashByte;
            uint8_t bufferByte;
            for (size_t i = 0; i < _sectorSize; i++)
            {
                flashByte = (flash() + _bufferSector * _sectorSize)[i];
                bufferByte = _buffer[i];

                if (bufferByte != flashByte && (bufferByte & ~flashByte))
                    return true;
            }

            return false;
        }

        void Base::loadSector(uint16_t sector, bool force /* = false */)
        {
            // skip - already loaded and not force
            if (!force && _buffer != nullptr && sector == _bufferSector)
                return;

            // load specific sector
            logTraceP("load buffer for sector %i", sector);
            logIndentUp();

            // an other sector is loaded - commit before load
            if (_buffer != nullptr && sector != _bufferSector)
                commit();

            // initalize buffer for first time
            if (_buffer == nullptr)
                _buffer = new uint8_t[_sectorSize];

            _bufferSector = sector;
            memcpy(_buffer, flash() + _bufferSector * _sectorSize, _sectorSize);
            logIndentDown();
        }

        void Base::commit()
        {
            // no sector loaded
            if (_buffer == nullptr)
                return;

            logTraceP("commit");
            logIndentUp();
            writeSector();
            logIndentDown();
        }

        uint32_t Base::write(uint32_t relativeAddress, uint8_t value, uint32_t size /* = 1 */)
        {
            if (size <= 0)
                return relativeAddress;

            uint16_t sector = sectorOfRelativeAddress(relativeAddress);

            // load buffer if needed
            loadSector(sector);

            // position in loaded buffer
            uint16_t bufferPosition = relativeAddress % _sectorSize;

            // determine available large within the sector
            uint16_t writeMaxSize = _sectorSize - bufferPosition;

            // determine size outside the sector
            uint16_t overheadSize = (writeMaxSize < size) ? size - writeMaxSize : 0;

            // determine how much must be stored within the sector.
            uint16_t writeSize = (writeMaxSize < size) ? writeMaxSize : size;

            // write date to current buffer
            memset(_buffer + bufferPosition, value, writeSize);

            // write overhead in next sector
            if (overheadSize > 0)
                return write(relativeAddress + writeSize, value, overheadSize);

            return relativeAddress + size;
        }

        uint32_t Base::write(uint32_t relativeAddress, uint8_t *buffer, uint32_t size /* = 1 */)
        {
            if (size <= 0)
                return relativeAddress;

            uint16_t sector = sectorOfRelativeAddress(relativeAddress);

            // load buffer if needed
            loadSector(sector);

            // position in loaded buffer
            uint16_t bufferPosition = relativeAddress % _sectorSize;

            // determine available large within the sector
            uint16_t writeMaxSize = _sectorSize - bufferPosition;

            // determine size outside the sector
            uint16_t overheadSize = (writeMaxSize < size) ? size - writeMaxSize : 0;

            // determine how much must be stored within the sector.
            uint16_t writeSize = (writeMaxSize < size) ? writeMaxSize : size;

            // write date to current buffer
            memcpy(_buffer + bufferPosition, buffer, writeSize);

            // write overhead in next sector
            if (overheadSize > 0)
                return write(relativeAddress + writeSize, buffer + writeSize, overheadSize);

            return relativeAddress + size;
        }

        uint32_t Base::writeByte(uint32_t relativeAddress, uint8_t value)
        {
            return write(relativeAddress, (uint8_t *)&value, 1);
        }

        uint32_t Base::writeWord(uint32_t relativeAddress, uint16_t value)
        {
            return write(relativeAddress, (uint8_t *)&value, 2);
        }

        uint32_t Base::writeInt(uint32_t relativeAddress, uint32_t value)
        {
            return write(relativeAddress, (uint8_t *)&value, 4);
        }

        uint32_t Base::read(uint32_t relativeAddress, uint8_t *output, uint32_t size)
        {
            memcpy(output, flash() + relativeAddress, size);
            return relativeAddress + 1;
        }

        uint8_t Base::readByte(uint32_t relativeAddress)
        {
            uint8_t buffer = 0;
            read(relativeAddress, &buffer, 1);
            return buffer;
        }

        uint16_t Base::readWord(uint32_t relativeAddress)
        {
            uint16_t buffer = 0;
            read(relativeAddress, (uint8_t *)&buffer, 2);
            return buffer;
        }

        uint32_t Base::readInt(uint32_t relativeAddress)
        {
            uint32_t buffer = 0;
            read(relativeAddress, (uint8_t *)&buffer, 4);
            return buffer;
        }
    } // namespace Flash
} // namespace OpenKNX