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
#ifdef ARDUINO_ARCH_ESP32
    #include <sdkconfig.h>
#endif

// "bootloader" and Console::resetToBootloader() exist: RP2040/RP2350 USB boot, ESP32-S3 ROM download mode.
#if defined(ARDUINO_ARCH_RP2040) || defined(CONFIG_IDF_TARGET_ESP32S3)
    #define OPENKNX_CONSOLE_BOOTLOADER
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
        OpenKnxFlash,
        Data // KNX parameters, OpenKNX save data and filesystem; unlike All it keeps the RP2040 EEPROM and firmware
    };

    class Console
    {
      private:
        uint8_t _consoleCharRepeats = 0;
        uint8_t _consoleCharLast = 0x0;
        bool _diagnoseKoOutput = false;
        bool _disableConsole = false;
        const char* _disableReason = nullptr; // why the console is silenced (e.g. "remote 1.0.241") -> shown once on local input
        uint32_t _disableNoticeMs = 0;        // rate-limit the "external console session active" notice so a held key / paste cannot spam
#ifdef OPENKNX_FTC_CONSOLE
        void (*_lineSink)(const char *) = nullptr; // FTC console tunnel: divert a finished line instead of running it locally
#endif
#ifdef KNX_HAS_TP
        bool _bcuDebug = false;
#endif

        void sleep();
        uint32_t sleepTime();
        void showWatchdogResets(bool diagnoseKo = false);
#if OPENKNX_LITTLE_FS
        void showFilesystem();
        void showFilesystemDirectory(std::string path);
        void showFilesystemUsage();
        void showFilesystemHelp();
#endif
#ifndef ARDUINO_ARCH_SAMD
        void processPinCommand(const std::string& cmd);
#endif
        void processI2cCommand(const std::string& cmd);

      public:
        char prompt[CONSOLE_INPUT_SIZE + 1] = {};
        void loop();
        /** @brief Erases the areas of mode, then restarts; blocks for the erase, a filesystem format and 1 s, never returns. */
        void erase(EraseMode mode = EraseMode::All);
#ifdef OPENKNX_CONSOLE_BOOTLOADER
        /** @brief Restarts into the USB boot (RP2040/RP2350) or ROM download mode (ESP32-S3); never returns. */
        void resetToBootloader();
#endif
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
#if defined(ARDUINO_ARCH_ESP32) && defined(OPENKNX_CONSOLE_MEM_DETAIL)
        /** @brief Unused stack bytes per task and the block count of the internal heap: where memory can be won. */
        void showMemoryTasks();
#endif
        void processSerialInput();
        void showInformations();
        void showVersions();
        void showUptime(bool diagnoseKo = false);
        void showMemoryContent(uint8_t* start, uint32_t size);
        void showMemoryLine(uint8_t* line, uint32_t length, uint8_t* memoryStart);

        void showHelp();
        void showMemHelp();

#ifdef KNX_HAS_TP
        bool bcuDebug()
        {
            return _bcuDebug;
        }
#endif

#ifdef BASE_KoDiagnose
        void processDiagnoseKo(GroupObject& ko);
        void writeDiagnoseKo(const char* message, ...);
        [[deprecated("Use writeDiagnoseKo(...) without the typo!")]] void writeDiagenoseKo(const char* message, ...);
#endif
    };
} // namespace OpenKNX