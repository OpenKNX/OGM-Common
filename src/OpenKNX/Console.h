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
#ifdef ARDUINO_ARCH_SAMD
    #define CONSOLE_INPUT_SIZE 14
#else
    #define CONSOLE_INPUT_SIZE 100
#endif

namespace OpenKNX
{

    enum class EraseMode
    {
        All,
        Filesystem,
        KnxFlash,
        OpenKnxFlash
    };

    class Console
    {
      private:
        uint8_t _consoleCharRepeats = 0;
        uint8_t _consoleCharLast = 0x0;
        bool _diagnoseKoOutput = false;
        bool _disableConsole = false;
        const char* _disableReason = nullptr; // why the console is silenced (e.g. "remote 1.0.241") -> shown once on local input
        uint32_t _disableNoticeMs = 0;        // rate-limit the "occupied" notice so a held key / paste cannot spam
#ifdef OPENKNX_FTC_CONSOLE
        void (*_lineSink)(const char *) = nullptr; // FTC console tunnel: divert a finished line instead of running it locally
#endif
#ifdef KNX_HAS_TP
        bool _bcuDebug = false;
#endif

        void sleep();
        uint32_t sleepTime();
        void showWatchdogResets(bool diagnoseKo = false);
#ifdef ARDUINO_ARCH_RP2040
        void resetToBootloader();
#endif
#if OPENKNX_LITTLE_FS
        void showFilesystem();
        void showFilesystemDirectory(std::string path);
        void showFilesystemUsage();
        void showFilesystemHelp();
#endif
        void erase(EraseMode mode = EraseMode::All);
#ifndef ARDUINO_ARCH_SAMD
        void processPinCommand(const std::string& cmd);
#endif
        void processI2cCommand(const std::string& cmd);

      public:
        char prompt[CONSOLE_INPUT_SIZE + 1] = {};
        void loop();
        void disableConsole(bool disable, const char* reason = nullptr);
        bool submitLine(const char* line);
#ifdef OPENKNX_FTC_CONSOLE
        // FTC console tunnel: while set, processSerialInput diverts each finished line to the sink (sent to
        // the remote device) instead of running it locally. nullptr = back to the local console.
        void setLineSink(void (*s)(const char *)) { _lineSink = s; }
#endif

        void printHelpLine(const char* command, const char* message);
        bool processCommand(std::string cmd, bool diagnoseKo = false);
        void showMemory(bool diagnoseKo = false);
        void processSerialInput();
        void showInformations();
        void showVersions();
        void showUptime(bool diagnoseKo = false);
        void showMemoryContent(uint8_t* start, uint32_t size);
        void showMemoryLine(uint8_t* line, uint32_t length, uint8_t* memoryStart);

        void showHelp();

#ifdef KNX_HAS_TP
        bool bcuDebug()
        {
            return _bcuDebug;
        }
#endif

#ifdef BASE_KoDiagnose
        void processDiagnoseKo(GroupObject& ko);
        void writeDiagnoseKo(const char* message, ...);
        void writeDiagenoseKo(const char* message, ...);
#endif
    };
} // namespace OpenKNX