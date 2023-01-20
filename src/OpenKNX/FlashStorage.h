#pragma once
#include <Helper.h>
#include "knx.h"
#include "knx/bits.h"
#include <stddef.h>

#ifndef FLASH_DATA_WRITE_LIMIT
#define FLASH_DATA_WRITE_LIMIT 180000 // 3 Minutes delay
#endif

#define FLASH_DATA_INIT 1330337281 // 4F 4B 56 01
#define FLASH_DATA_INIT_LEN 4
#define FLASH_DATA_MODULE_ID_LEN 1
#define FLASH_DATA_SIZE_LEN 2
#define FLASH_DATA_CHK_LEN 2
#define FLASH_DATA_APP_LEN 4
#define FLASH_DATA_META_LEN (FLASH_DATA_APP_LEN + FLASH_DATA_SIZE_LEN + FLASH_DATA_CHK_LEN + FLASH_DATA_INIT_LEN)

namespace OpenKNX
{
    class FlashStorage
    {
      public:
        FlashStorage();
        ~FlashStorage();

        void load();
        void readData(bool *loadedModules);
        void initUnloadedModules(bool *loadedModules);
        void save(bool force = false);

        void write(uint8_t *buffer, uint16_t size = 1);
        void write(uint8_t value, uint16_t size);
        void writeByte(uint8_t value);
        void writeWord(uint16_t value);
        void writeInt(uint32_t value);

        uint16_t applicationVersion();

      private:
        uint8_t *_flashStart;
        uint16_t _flashSize = 0;
        uint32_t _lastWrite = 0;
        uint8_t _lastOpenKnxId = 0;
        uint8_t _lastApplicationNumber = 0;
        uint16_t _lastApplicationVersion = 0;

        uint16_t _checksum = 0;
        uint32_t _currentWriteAddress = 0;
        uint32_t _maxWriteAddress = 0;

        void zeroize();
        uint16_t calcChecksum(uint8_t *data, uint16_t size);
        uint16_t calcChecksum(uint16_t data);
        bool verifyChecksum(uint8_t *data, uint16_t size);
    };
} // namespace OpenKNX