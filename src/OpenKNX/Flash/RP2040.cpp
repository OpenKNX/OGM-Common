#ifdef ARDUINO_ARCH_RP2040

#include "OpenKNX/Flash/RP2040.h"
#include "OpenKNX/Common.h"

extern uint32_t _EEPROM_start;
extern uint32_t _FS_start;
extern uint32_t _FS_end;
namespace OpenKNX
{
    namespace Flash
    {
        RP2040::RP2040(uint32_t startOffset, uint32_t size, std::string id)
        {
            _id = id;
            _startOffset = startOffset;
            _size = size;
            _sectorSize = FLASH_SECTOR_SIZE;
            // Full Size
            // _maxSize = (uint32_t)(&_EEPROM_start) - 0x10000000lu + 4096lu;
            // Size up to EEPROM
            // _maxSize = (uint32_t)(&_EEPROM_start) - 0x10000000lu;
            // Size up to FS (if FS 0 it = _EEPROM_start)
            _maxSize = (uint32_t)(&_FS_start) - 0x10000000lu;

            validateParameters();
        }

        uint8_t* RP2040::flash()
        {
            return (uint8_t*)XIP_BASE + _startOffset;
        }

        void RP2040::eraseSector(uint16_t sector)
        {
            if (!needEraseSector(sector))
            {
                logTraceP("skip erase sector, because already erased");
                return;
            }

            logTraceP("erase sector %i", sector);
            noInterrupts();
            rp2040.idleOtherCore();
            flash_range_erase(_startOffset + (sector * _sectorSize), _sectorSize);
            rp2040.resumeOtherCore();
            interrupts();
        }

        void RP2040::writeSector()
        {
            if (!needWriteSector())
            {
                logTraceP("skip write sector, because no changes");
                return;
            }

            if (needEraseForBuffer())
            {
                eraseSector(_bufferSector);
            }

            logTraceP("write sector %i", _bufferSector);
            // logHexTraceP(_buffer, _sectorSize);

            noInterrupts();
            rp2040.idleOtherCore();

            // write smaller FLASH_PAGE_SIZE to reduze write time
            uint32_t currentPosition = 0;
            uint32_t currentSize = 0;
            while (currentPosition < _sectorSize)
            {
                while (memcmp(_buffer + currentPosition + currentSize, flash() + (_bufferSector * _sectorSize) + currentPosition + currentSize, FLASH_PAGE_SIZE))
                {
                    currentSize += FLASH_PAGE_SIZE;

                    // last
                    if (currentPosition + currentSize == _sectorSize)
                        break;
                }

                // Changes Found
                if (currentSize > 0)
                    flash_range_program(_startOffset + (_bufferSector * _sectorSize) + currentPosition, _buffer + currentPosition, currentSize);

                currentPosition += currentSize + FLASH_PAGE_SIZE;
                currentSize = 0;
            }
            rp2040.resumeOtherCore();
            interrupts();
        }
    } // namespace Storage
} // namespace OpenKNX
#endif