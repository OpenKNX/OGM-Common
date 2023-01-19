#pragma once
#include "OpenKNX.h"
// #include "knxprod.h"
// #include <cstdio>
// #include <functional>

namespace OpenKNX
{
    class Module
    {
      protected:
        uint8_t mChannelIndex = 0;
        uint16_t mChannelParamBlockSize = 0;
        uint16_t mChannelParamOffset = 0;
        uint16_t mChannelParamKoBlockSize = 0;
        uint16_t mChannelParamKoOffset = 0;
        bool _flashLoaded = false;

        uint32_t calcParamIndex(uint16_t iParamIndex);
        uint16_t calcKoNumber(uint8_t iKoIndex);
        int8_t calcKoIndex(uint16_t iKoNumber);
        GroupObject* getKo(uint8_t iKoIndex);

      public:
        int debug(const char* output, ...);
        virtual const char* name();
        virtual const char* version();
        virtual void loop();
        virtual void setup();
        virtual void processInputKo(GroupObject& iKo);
        virtual void processSavePin();
        virtual void firstLoop();
        virtual void processBeforeRestart();
        virtual void processBeforeTablesUnload();

        virtual void writeFlash()
        {}

        virtual void readFlash(const uint8_t* data, const uint16_t size)
        {
            _flashLoaded = true;
        }

        virtual uint16_t flashSize()
        {
            return 0;
        }

        bool flashLoaded()
        {
            return _flashLoaded;
        }
    };
} // namespace OpenKNX