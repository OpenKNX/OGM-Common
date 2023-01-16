#pragma once
#include "Helper.h"
#include "knx.h"
#include <stddef.h>

#ifndef FLASH_USER_DATA_WRITE_LIMIT
#define FLASH_USER_DATA_WRITE_LIMIT 180000 // 3 Minutes delay
#endif

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
        uint16_t cachedUserDataSize = 0;

        uint8_t *newUserData();
        uint16_t userDataSize();
        uint8_t calcChecksum(uint8_t *data, uint16_t len);
        bool validateChecksum(uint8_t *data, uint16_t len);
    };
} // namespace OpenKNX