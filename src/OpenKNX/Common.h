#pragma once
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Log/VirtualSerial.h"
#ifdef OPENKNX_RUNTIME_STAT
    #include "OpenKNX/Stat/RuntimeStat.h"
#endif
#include "OpenKNX/Led/FunctionManager.h"
#include "OpenKNX/Led/Functions.h"
#include "OpenKNX/defines.h"
#include "knx.h"

namespace OpenKNX
{

    class Common
    {
      private:
        OpenKNX::Led::FunctionGroup* _progLedFunc = nullptr;
        OpenKNX::Led::FunctionGroup* _stateLedFunc = nullptr;

        uint32_t _unsupportedEtsModules = 0;
#if OPENKNX_LOOPTIME_WARNING > 1
        uint32_t _lastLooptimeWarning = 0;
        bool _skipLooptimeWarning = false;
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
#ifdef ARDUINO_ARCH_ESP32
        volatile int32_t _freeStackMin = 0;
    #ifdef OPENKNX_DUALCORE
        volatile int32_t _freeStackMin1 = 0;
    #endif
#endif
#ifdef ARDUINO_ARCH_RP2040
        volatile int32_t _freeStackMin = 0x2000;
    #ifdef OPENKNX_DUALCORE
        volatile int32_t _freeStackMin1 = 0x2000;
    #endif
#endif
        Led::Functions _ledFunctions = Led::Functions();

        // propertyFunctionWrapper global storage
        uint8_t _apduLength = 0;
        uint16_t _packageCount = 0;
        int8_t _sequenceNumber = 0;
        uint8_t _receivedData[256];
        uint8_t _receivedLength;
        uint8_t _resultData[256];
        int16_t _resultLength = 0;


        void initKnx();

        void processModulesLoop();
        void registerCallbacks();
        void processRestoreSavePin();
        void initMemoryTimerInterrupt();
        void debugWait();

#ifdef OPENKNX_DEBUG
        void showDebugInfo();
#endif

#if defined(PROG_BUTTON_PIN) && PROG_BUTTON_PIN >= 0 && OPENKNX_RECOVERY_TIME > 0
        void processRecovery();
#endif

#ifdef BASE_HeartbeatDelayBase
        uint32_t _heartbeatDelay = 0;
        void processHeartbeat();
#endif
#ifdef BASE_PeriodicSave
        void processPeriodicSave();
#endif

        bool processFunctionPropertyWrapper(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength);
        bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength);
        bool processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t* data, uint8_t* resultData, uint8_t& resultLength);

#ifdef OPENKNX_RUNTIME_STAT
        Stat::RuntimeStat _runtimeLoop;
        Stat::RuntimeStat _runtimeConsole;
        Stat::RuntimeStat _runtimeKnxStack;
        Stat::RuntimeStat _runtimeModuleLoop;
        Stat::RuntimeStat _runtimeGPIO;
        Stat::RuntimeStat _runtimeTimeManager;
    #ifdef ParamBASE_Latitude
        Stat::RuntimeStat _runtimeSunCalculation;
    #endif
#endif

#ifdef BASE_StartupDelayBase
        uint32_t _startupDelay = 0;
        bool _firstStartup = true;
#endif
        bool _afterStartupDelay = false;

#ifdef BASE_KoManualSave
        void processSaveKo(GroupObject& ko);
#endif

      public:
        /*
         * Internal public api
         */
        uint8_t extendedHeartbeatValue = 1;
        static VersionCheckResult versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion);

        void init(uint8_t firmwareRevision);
        void triggerSavePin();
        void setup();
        void loop();

#ifdef OPENKNX_DUALCORE
        void setup1();
        void loop1();
#endif

        bool afterStartupDelay();
        void processAfterStartupDelay();
        void skipLooptimeWarning();
        void restart();

        void collectHeapStats();
        void collectStackStats();
        uint freeMemoryMin();
#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_ESP32)
        int freeStackMin();
    #ifdef OPENKNX_DUALCORE
        int freeStackMin1();
    #endif
#endif
        bool freeLoopTime();
        bool freeLoopIterate(uint8_t size, uint8_t& position, uint8_t& processed);
        void unsupportedEtsModule(uint8_t etsModuleId);

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
