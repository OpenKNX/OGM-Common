#pragma once
#include <Arduino.h>
#include <string>
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

#ifdef ARDUINO_ARCH_RP2040
    enum class EraseMode
    {
        All,
        Filesystem,
        KnxFlash,
        OpenKnxFlash
    };
#endif

    class Console
    {
      private:
        uint8_t _consoleCharRepeats = 0;
        uint8_t _consoleCharLast = 0x0;

        void processSerialInput();
        void showInformations();
        void showVersions();
        void showMemory();

        bool confirmation(char key, uint8_t repeats, const char* message);
        void showHelp();
        void sleep();
        uint32_t sleepTime();
        void processCommand(std::string cmd);
#ifdef ARDUINO_ARCH_RP2040
        void resetToBootloader();
        void showFilesystem();
        void showFilesystemDirectory(std::string path);
        void erase(EraseMode mode = EraseMode::All);
#endif

      public:
        char prompt[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        void loop();

        void printHelpLine(const char* command, const char* message);
    };
} // namespace OpenKNX