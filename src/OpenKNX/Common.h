#pragma once
#include "../Helper.h"
#include "KnxHelper.h"
#include "OpenKNX/FlashStorage.h"
#include "OpenKNX/Helper.h"
#include "OpenKNX/Module.h"
#include "hardware.h"
#include "knxprod.h"
#include <knx.h>

#ifndef OPENKNX_MAX_MODULES
#define OPENKNX_MAX_MODULES 9
#endif

namespace OpenKNX
{
    struct Modules
    {
        uint8_t count = 0;
        uint8_t ids[OPENKNX_MAX_MODULES];
        Module* list[OPENKNX_MAX_MODULES];
    };

    class Common : public Helper
    {
      private:
#ifdef DEBUG_LOOP_TIME
        uint32_t lastDebugTime = 0;
#endif
        uint8_t _firmwareRevision = 0;
        Modules modules;

        bool _firstLoopProcessed = false;
        uint _freeMemoryMin = -1;
        uint _freeMemoryMax = 0;
        void initKnx();
        void appSetup();
        void appLoop();
        void loopModule(uint8_t id);
        void processFirstLoop();
        void processModulesLoop();
        void registerCallbacks();
        void processSerialInput();
#ifdef LOG_StartupDelayBase
        uint32_t _startupDelay;
        bool processStartupDelay();
#endif
#ifdef LOG_HeartbeatDelayBase
        uint32_t _heartbeatDelay;
        void processHeartbeat();
#endif

      public:
        bool _save = false;
        bool _saved = false;
        FlashStorage flash;

        Common();
        ~Common();

        static VersionCheckResult versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion);

        void init(uint8_t firmwareRevision);
        void setup();
        void loop();
        void addModule(uint8_t id, Module* module);
        void collectMemoryStats();
        void showMemoryStats();
        Module* getModule(uint8_t id);
        Modules* getModules();

        void processSavePin();
        void processBeforeRestart();
        void processBeforeTablesUnload();
        void processInputKo(GroupObject& iKo);

        uint8_t openKnxId();
        uint8_t applicationNumber();
        uint16_t applicationVersion();
        const char* applicationHumanVersion();
    };
} // namespace OpenKNX

extern OpenKNX::Common openknx;
