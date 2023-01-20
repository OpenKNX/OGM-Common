#pragma once
#include "OpenKNX/Module.h"

namespace OpenKNX
{
    class Channel : public Module
    {
      protected:
        uint8_t _channelIndex = 0;
        uint16_t _channelParamBlockSize = 0;
        uint16_t _channelParamOffset = 0;
        uint16_t _channelParamKoBlockSize = 0;
        uint16_t _channelParamKoOffset = 0;

        uint32_t calcParamIndex(uint16_t paramIndex);
        uint16_t calcKoNumber(uint8_t koIndex);
        int8_t calcKoIndex(uint16_t koNumber);
        GroupObject* getKo(uint8_t koIndex);

      public:
        virtual const char* name();
      
    };
} // namespace OpenKNX