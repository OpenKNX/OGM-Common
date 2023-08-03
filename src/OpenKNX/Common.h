#pragma once
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Log/VirtualSerial.h"
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
        uint32_t _lastLoopOutput = 0;
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
#ifdef LOG_HeartbeatDelayBase
        uint32_t _heartbeatDelay;
        void processHeartbeat();
#endif
        bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength);
        bool processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength);

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

#if OPENKNX_LOOPTIME_WARNING > 1
        void resetLastLoopOutput();
#endif
#ifdef LOG_StartupDelayBase
        uint32_t _startupDelay;
#endif
#ifdef OPENKNX_WATCHDOG
        void watchdogLoop();
#endif
        bool _afterStartupDelay = false;
        bool afterStartupDelay();
        void processAfterStartupDelay();

        void collectMemoryStats();
        uint freeMemoryMin();
        bool freeLoopTime();

        void processSavePin();
        void processBeforeRestart();
        void processBeforeTablesUnload();
#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
        void processInputKo(GroupObject& ko);
#endif
        std::string logPrefix();
    };
} // namespace OpenKNX
