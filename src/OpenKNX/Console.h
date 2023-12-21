#pragma once
#include "OpenKNX/defines.h"
#include "knx.h"
#include <string>
#ifdef WATCHDOG
    #include <Adafruit_SleepyDog.h>
#endif
#ifdef ARDUINO_ARCH_RP2040
extern "C"
{
    #include "pico/bootrom.h"
}
#endif

#define CONSOLE_HEADLINE_COLOR 33

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
        bool _diagnoseKoOutput = false;

        void processSerialInput();
        void showInformations();
        void showVersions();
        void showUptime(bool diagnoseKo = false);
        void showMemory(bool diagnoseKo = false);
        void showMemoryContent(uint8_t* start, uint32_t size);
        void showMemoryLine(uint8_t* line, uint32_t length, uint8_t* memoryStart);

        void showHelp();
        void sleep();
        uint32_t sleepTime();
        void showWatchdogResets(bool diagnoseKo = false);
#ifdef ARDUINO_ARCH_RP2040
        void resetToBootloader();
        void showFilesystem();
        void showFilesystemDirectory(std::string path);
        void erase(EraseMode mode = EraseMode::All);
#endif

      public:
        char prompt[15] = {};
        void loop();

        void printHelpLine(const char* command, const char* message);
        bool processCommand(std::string cmd, bool diagnoseKo = false);
#ifdef BASE_KoDiagnose
        void processDiagnoseKo(GroupObject& ko);
        void writeDiagenoseKo(const char* message, ...);
#endif
    };
} // namespace OpenKNX