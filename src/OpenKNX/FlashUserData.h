#pragma once
#include "Helper.h"
#include "knx.h"
#include <stddef.h>

#ifndef FLASH_USER_DATA_WRITE_LIMIT
#define FLASH_USER_DATA_WRITE_LIMIT 180000 // 3 Minutes delay
#endif

#define FLASH_USER_DATA_MAGICWORD_LEN 4
#define FLASH_USER_DATA_SIZE_LEN 2
#define FLASH_USER_DATA_CHK_LEN 1

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
        uint8_t calcChecksum(uint8_t *data, uint16_t len);
        bool verifyChecksum(uint8_t *data, uint16_t len);
    };
} // namespace OpenKNX