#pragma once
#include "Helper.h"
#include "KnxHelper.h"
#include "OpenKNX/FlashUserData.h"
#include "OpenKNX/Module.h"
#include <knx.h>

#ifndef OPENKNX_MAX_MODULES
#define OPENKNX_MAX_MODULES 9
#endif

namespace OpenKNX
{
    struct Modules
    {
        uint8_t count = 0;
        Module* list[OPENKNX_MAX_MODULES];
    };

    class Common
    {
      private:
#ifdef DEBUG_LOOP_TIME
        uint32_t lastDebugTime = 0;
#endif
        uint8_t firmwareRevision = 0;
        Modules modules;
        FlashUserData* flashUserData;

        bool firstLoopProcessed = false;
        void processSerialInput();
        void initKnx();
        void appSetup();
        void appLoop();
        void processFirstLoop();
        void processModulesLoop();
        void registerCallbacks();
#ifdef LOG_StartupDelayBase
        uint32_t startupDelay;
        bool processStartupDelay();
#endif
#ifdef LOG_HeartbeatDelayBase
        uint32_t heartbeatDelay;
        void processHeartbeat();
#endif

      public:
        bool save = false;
        bool saved = false;

        Common();
        ~Common();

        static VersionCheckResult versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion);

        void init(uint8_t firmwareRevision);
        uint16_t version();
        void setup();
        void loop();
        void addModule(Module* module);
        Modules* getModules();

        void processSavePin();
        void processBeforeRestart();
        void processBeforeTablesUnload();
        void processInputKo(GroupObject& iKo);
        int debug(const char* prefix, const char* output, ...);
    };
} // namespace OpenKNX

extern OpenKNX::Common openknx;