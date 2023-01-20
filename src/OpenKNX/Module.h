#pragma once
#include <knx.h>

namespace OpenKNX
{
    class Module
    {
      protected:
        virtual GroupObject* getKo(uint16_t koNumber);

      public:
        void debug(const char* output, ...);
        virtual const char* name();
        virtual const char* version();
        virtual void loop();
        virtual void setup();
        virtual void processInputKo(GroupObject& ko);
        virtual void processSavePin();
        virtual void firstLoop();
        virtual void processBeforeRestart();
        virtual void processBeforeTablesUnload();
        virtual void writeFlash();
        virtual void readFlash(const uint8_t* data, const uint16_t size);
        virtual uint16_t flashSize();
    };
} // namespace OpenKNX