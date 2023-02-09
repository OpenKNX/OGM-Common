#pragma once
#include "knx.h"
#ifdef WATCHDOG
#include <Adafruit_SleepyDog.h>
#endif

namespace OpenKNX
{
    class Console
    {
      private:
        uint8_t _consoleCharRepeats = 0;
        uint8_t _consoleCharLast = 0x0;

        void processSerialInput();
        void showInformations();

      public:
        void loop();
    };
} // namespace OpenKNX