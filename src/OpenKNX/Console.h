#pragma once
#include "knx.h"
#ifdef WATCHDOG
#include <Adafruit_SleepyDog.h>
#endif
#ifdef ARDUINO_ARCH_RP2040
extern "C" {
#include "pico/bootrom.h"
}
#include "LittleFS.h"
#endif

#define CONSOLE_HEADLINE_COLOR 11

namespace OpenKNX
{
    class Console
    {
      private:
        uint8_t _consoleCharRepeats = 0;
        uint8_t _consoleCharLast = 0x0;

        void processSerialInput();
        void showInformations();

        bool confirmation(char key, uint8_t repeats, const char* message);
        void showHelp();
        void sleep();
        uint32_t sleepTime();
        void nukeFlash();
        void nukeFlashKnxOnly();
#ifdef ARDUINO_ARCH_RP2040
        void resetToBootloader();
        void showFilesystem();
        void showFilesystemDirectory(std::string path);
#endif

      public:
        void loop();
    };
} // namespace OpenKNX