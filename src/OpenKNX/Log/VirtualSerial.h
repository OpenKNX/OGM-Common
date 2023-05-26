#pragma once
#include "Arduino.h"
#include "OpenKNX/Log/Logger.h"

namespace OpenKNX
{
    namespace Log
    {
        class VirtualSerial : public arduino::Stream
        {
          private:
            const char* _prefix;
            std::string _buffer;

          public:
            VirtualSerial(const char* prefix, uint16_t reserveSize = 100);
            int available() override;
            int read() override;
            int peek() override;
            size_t write(uint8_t byte) override;
        };
    } // namespace Log
} // namespace OpenKNX