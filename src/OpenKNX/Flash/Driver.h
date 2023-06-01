#pragma once
#include <Arduino.h>
#include <string>

namespace OpenKNX
{
    namespace Flash
    {
        class Driver
        {
          protected:
            std::string _id = "Unnamed";

            uint32_t _offset = 0;
            uint32_t _size = 0;
            uint32_t _startFree = 0;
            uint32_t _endFree = 0;
            uint16_t _sectorSize = 0;

            uint8_t *_buffer = nullptr;
            uint16_t _bufferSector = 0;

            void writeSector();
            bool needWriteSector();
            bool needEraseForBuffer();
            bool needEraseSector(uint16_t sector = 0);
            uint16_t sectorOfRelativeAddress(uint32_t relativeAddress);

            void validateParameters();

            void loadSector(uint16_t sector, bool force = false);

          public:
            Driver(uint32_t offset, uint32_t size, std::string id);
            std::string logPrefix();

            void eraseSector(uint16_t sector = 0);
            uint8_t *flashAddress();

            void commit();
            uint32_t size();
            uint32_t startFree();
            uint32_t endFree();
            uint32_t sectorSize();
            uint32_t startOffset();
            void printBaseInfo();

            uint32_t write(uint32_t relativeAddress, uint8_t value, uint32_t size = 1);
            uint32_t write(uint32_t relativeAddress, uint8_t *buffer, uint32_t size = 1);

            uint32_t writeByte(uint32_t relativeAddress, uint8_t value);
            uint32_t writeWord(uint32_t relativeAddress, uint16_t value);
            uint32_t writeInt(uint32_t relativeAddress, uint32_t value);

            uint32_t read(uint32_t relativeAddress, uint8_t *output, uint32_t size);

            uint8_t readByte(uint32_t relativeAddress);
            uint16_t readWord(uint32_t relativeAddress);
            uint32_t readInt(uint32_t relativeAddress);
        };
    } // namespace Flash
} // namespace OpenKNX
