#pragma once
#include "knx.h"
#ifdef WATCHDOG
#include <Adafruit_SleepyDog.h>
#endif
#ifdef ARDUINO_ARCH_RP2040
extern "C" {
#include "pico/bootrom.h"
}
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
        void showHelp();
#ifdef ARDUINO_ARCH_RP2040
        void resetToBootloader();
#endif

      public:
        void loop();
    };
} // namespace OpenKNX