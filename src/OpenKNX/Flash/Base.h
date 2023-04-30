#pragma once
#include "knx.h"
#include "knx/bits.h"
#include <string>

namespace OpenKNX
{
    namespace Flash
    {
        class Base
        {
          protected:
            std::string _id = "Unnamed";
            
            uint32_t _startOffset = 0;
            uint32_t _size = 0;
            uint32_t _maxSize = 0;
            uint16_t _sectorSize = 0;

            uint8_t *_buffer = nullptr;
            uint16_t _bufferSector = 0;

            virtual void writeSector(){};
            bool needWriteSector();
            bool needEraseForBuffer();
            bool needEraseSector(uint16_t sector = 0);
            uint16_t sectorOfRelativeAddress(uint32_t relativeAddress);

            void validateParameters();
            void validateSize();
            void validateOffset();

            void loadSector(uint16_t sector, bool force = false);
            void loadSectorBufferAndCommit(uint16_t sector);

          public:
            std::string logPrefix();

            // depend on hw
            virtual void eraseSector(uint16_t sector = 0){};
            virtual uint8_t *flash() { return nullptr; };

            void commit();
            uint32_t size();
            uint32_t maxSize();
            uint32_t sectorSize();
            uint32_t startOffset();
            void printBaseInfo();

            uint32_t write(uint32_t relativeAddress, uint8_t value, uint32_t size = 1);
            uint32_t write(uint32_t relativeAddress, uint8_t *buffer, uint32_t size = 1);
            uint32_t write(uint32_t relativeAddress, int8_t value);
            uint32_t write(uint32_t relativeAddress, uint16_t value);
            uint32_t write(uint32_t relativeAddress, int16_t value);
            uint32_t write(uint32_t relativeAddress, uint32_t value);
            uint32_t write(uint32_t relativeAddress, int32_t value);

            uint32_t writeByte(uint32_t relativeAddress, uint8_t value);
            uint32_t writeWord(uint32_t relativeAddress, uint16_t value);
            uint32_t writeInt(uint32_t relativeAddress, uint32_t value);

            uint8_t *read(uint32_t relativeAddress);
            uint32_t read(uint32_t relativeAddress, uint8_t &output);
            uint32_t read(uint32_t relativeAddress, int8_t &output);
            uint32_t read(uint32_t relativeAddress, uint16_t &output);
            uint32_t read(uint32_t relativeAddress, int16_t &output);
            uint32_t read(uint32_t relativeAddress, uint32_t &output);
            uint32_t read(uint32_t relativeAddress, int32_t &output);

            uint8_t readByte(uint32_t relativeAddress);
            uint16_t readWord(uint32_t relativeAddress);
            uint32_t readInt(uint32_t relativeAddress);
        };
    } // namespace Storage
} // namespace OpenKNX
