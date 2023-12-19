#pragma once
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Log/VirtualSerial.h"
#ifdef OPENKNX_RUNTIME_STAT
    #include "OpenKNX/Stat/RuntimeStat.h"
#endif
#include "OpenKNX/defines.h"
#include "knx.h"

#ifdef OPENKNX_WATCHDOG
    #include <Adafruit_SleepyDog.h>
#endif

namespace OpenKNX
{
#ifdef OPENKNX_WATCHDOG
    struct WatchdogData
    {
        uint32_t timer = 0;
        uint8_t resetCause;
    };
#endif

    class Common
    {
      private:
#if OPENKNX_LOOPTIME_WARNING > 1
        uint32_t _lastLooptimeWarning = 0;
        bool _skipLooptimeWarning = false;
#endif
#ifdef OPENKNX_WATCHDOG
    #ifndef OPENKNX_WATCHDOG_MAX_PERIOD
        #define OPENKNX_WATCHDOG_MAX_PERIOD 16384
    #endif
        WatchdogData watchdog;
#endif
        uint8_t _currentModule = 0;
        uint32_t _loopMicros = 0;
        volatile bool _setup0Ready = false;
#ifdef OPENKNX_DUALCORE
        volatile bool _setup1Ready = false;
#endif

        uint32_t _savedPinProcessed = 0;
        bool _savePinTriggered = false;
        volatile int32_t _freeMemoryMin = 0x7FFFFFFF;
#ifdef ARDUINO_ARCH_RP2040
        volatile int32_t _freeStackMin = 0x2000;
    #ifdef OPENKNX_DUALCORE
        volatile int32_t _freeStackMin1 = 0x2000;
    #endif
#endif

        void initKnx();

        void processModulesLoop();
        void registerCallbacks();
        void processRestoreSavePin();
        void initMemoryTimerInterrupt();
        void debugWait();
#ifdef OPENKNX_DEBUG
        void showDebugInfo();
#endif
#if defined(ARDUINO_ARCH_RP2040) && defined(OPENKNX_RECOVERY_ON)
        void processRecovery();
#endif
#ifdef OPENKNX_WATCHDOG
        void watchdogSetup();
#endif
#ifdef BASE_HeartbeatDelayBase
        uint32_t _heartbeatDelay = 0;
        void processHeartbeat();
#endif
        bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength);
        bool processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength);

#ifdef OPENKNX_RUNTIME_STAT
        Stat::RuntimeStat _runtimeLoop;
        Stat::RuntimeStat _runtimeConsole;
        Stat::RuntimeStat _runtimeKnxStack;
        Stat::RuntimeStat _runtimeModuleLoop;
#endif

      public:
        static VersionCheckResult versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion);

        void init(uint8_t firmwareRevision);
        void triggerSavePin();
        void setup();
        void loop();

#ifdef OPENKNX_DUALCORE
        void setup1();
        void loop1();
#endif

#ifdef BASE_StartupDelayBase
        uint32_t _startupDelay = 0;
        bool _firstStartup = true;
#endif
        bool _watchdogRebooted = false;
#ifdef OPENKNX_WATCHDOG
        void watchdogLoop();
        uint8_t watchdogRestarts();
#endif
        bool _afterStartupDelay = false;
        bool afterStartupDelay();
        void processAfterStartupDelay();
        void skipLooptimeWarning();
        void restart();

        void collectMemoryStats();
        uint freeMemoryMin();
#ifdef ARDUINO_ARCH_RP2040
        int freeStackMin();
    #ifdef OPENKNX_DUALCORE
        int freeStackMin1();
    #endif
#endif
        bool freeLoopTime();
        bool freeLoopIterate(uint8_t size, uint8_t& position, uint8_t& processed);

        void processSavePin();
        void processBeforeRestart();
        void processBeforeTablesUnload();
#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
        void processInputKo(GroupObject& ko);
#endif
        std::string logPrefix();

#ifdef OPENKNX_RUNTIME_STAT
        void showRuntimeStat(const bool stat = true, const bool hist = false);
#endif
    };
} // namespace OpenKNX
