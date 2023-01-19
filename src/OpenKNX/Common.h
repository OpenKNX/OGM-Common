#pragma once
#include "Helper.h"
#include "KnxHelper.h"
#include "OpenKNX/FlashStorage.h"
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
        uint8_t ids[OPENKNX_MAX_MODULES];
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
        FlashStorage flash;

        Common();
        ~Common();

        static VersionCheckResult versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion);

        void init(uint8_t firmwareRevision);
        void setup();
        void loop();
        void addModule(uint8_t id, Module* module);
        Module* getModule(uint8_t id);
        Modules* getModules();
        FlashStorage &flash2();

        void processSavePin();
        void processBeforeRestart();
        void processBeforeTablesUnload();
        void processInputKo(GroupObject& iKo);
        int debug(const char* prefix, const char* output, ...);

        uint8_t openKnxId();
        uint8_t applicationNumber();
        uint16_t applicationVersion();
    };
} // namespace OpenKNX

extern OpenKNX::Common openknx;
