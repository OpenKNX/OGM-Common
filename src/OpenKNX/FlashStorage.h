#pragma once
#include "knx.h"
#include "knx/bits.h"
// #include <stddef.h>

#ifndef FLASH_DATA_WRITE_LIMIT
#define FLASH_DATA_WRITE_LIMIT 180000 // 3 Minutes delay
#endif

#define FLASH_DATA_FILLBYTE 0xFF

#define FLASH_DATA_INIT 1330337281 // 4F 4B 56 01
#define FLASH_DATA_INIT_LEN 4
#define FLASH_DATA_MODULE_ID_LEN 1
#define FLASH_DATA_SIZE_LEN 2
#define FLASH_DATA_CHK_LEN 2
#define FLASH_DATA_APP_LEN 4
#define FLASH_DATA_META_LEN (FLASH_DATA_APP_LEN + FLASH_DATA_SIZE_LEN + FLASH_DATA_CHK_LEN + FLASH_DATA_INIT_LEN)

namespace OpenKNX
{
    /**
     * OpenKnx FlashStorage for (Runtime) Data owned by Modules
     * 
     * Introduced with OpenKnx-Commons v2 to allow flexible extension.
     * 
     * Scope/Exclusion: ETS-Parametrization is NOT part of this data. Only e.g. values of Logic-Module, 
     *                  or calibration-data of sensors which should be saved on power loss.
     */
    class FlashStorage
    {
      public:
        FlashStorage();

        void load();
        void save(bool force = false);
        void write(uint8_t *buffer, uint16_t size = 1);
        void write(uint8_t value, uint16_t size);
        void writeByte(uint8_t value);
        void writeWord(uint16_t value);
        void writeInt(uint32_t value);
        uint8_t *read(uint16_t size = 1);
        uint8_t readByte();
        uint16_t readWord();
        uint32_t readInt();
        uint16_t firmwareVersion();

      private:
        bool *loadedModules;
        uint8_t *_flashStart;
        uint16_t _flashSize = 0;
        uint32_t _lastWrite = 0;
        uint16_t _lastFirmwareNumber = 0;
        uint16_t _lastFirmwareVersion = 0;
        uint16_t _checksum = 0;
        uint32_t _currentWriteAddress = 0;
        uint8_t *_currentReadAddress = 0;
        uint32_t _maxWriteAddress = 0;
        void zeroize();
        void readData();
        void initUnloadedModules();
        uint16_t calcChecksum(uint8_t *data, uint16_t size);
        uint16_t calcChecksum(uint16_t data);
        bool verifyChecksum(uint8_t *data, uint16_t size);
    };
} // namespace OpenKNX