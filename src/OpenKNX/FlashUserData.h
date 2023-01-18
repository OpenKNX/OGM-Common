#pragma once
#include "Helper.h"
#include "knx.h"
#include <stddef.h>

#ifndef FLASH_USER_DATA_WRITE_LIMIT
#define FLASH_USER_DATA_WRITE_LIMIT 180000 // 3 Minutes delay
#endif

#define FLASH_USER_DATA_MAGICWORD_LEN 4
#define FLASH_USER_DATA_MODULE_ID_LEN 1
#define FLASH_USER_DATA_SIZE_LEN 2
#define FLASH_USER_DATA_CHK_LEN 1
#define FLASH_USER_DATA_META_LEN (FLASH_USER_DATA_SIZE_LEN + FLASH_USER_DATA_CHK_LEN + FLASH_USER_DATA_MAGICWORD_LEN)

namespace OpenKNX
{
    class FlashUserData
    {
      public:
        FlashUserData();
        ~FlashUserData();

        void load();
        void save(bool force = false);

      private:
        uint8_t *flashStart;
        size_t flashSize = 0;
        uint32_t lastWrite = 0;

        uint8_t *initUserData(uint16_t len);
        uint16_t calcUserDataSize();
        void writeChecksum(uint8_t *data, uint16_t len);
        uint32_t writeFlash(uint32_t relativeAddress, uint8_t *buffer, size_t size = 1);
        uint32_t writeFlash(uint32_t relativeAddress, uint8_t data);
        uint32_t writeFlash(uint32_t relativeAddress, uint16_t data);
        uint8_t calcChecksum(uint8_t *data, uint16_t len);
        uint8_t calcChecksum(uint16_t data);
        bool verifyChecksum(uint8_t *data, uint16_t len);
    };
} // namespace OpenKNX