#pragma once
#include "Helper.h" // TODO: Hier ist das delayChecks drinne (Logicmodul)
#include "KnxHelper.h"
#include "OpenKNX/Module.h"
#include "knxprod.h"
#include <cstdio>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include "FlashUserData.h"

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
        uint8_t firmwareRevision = 0;
        Modules modules;

        bool firstLoop = false;
        void processSerialInput();

      public:
        bool save = false;
        bool saved = false;

        Common();
        ~Common();

        static VersionCheckResult versionCheck(uint16_t manufacturerId, uint8_t* hardwareType, uint16_t firmwareVersion);

        void init(uint8_t firmwareRevision);
        void initKnx();

        void setup();
        void appSetup();

        void loop();
        void appLoop();

        void addModule(Module* module);

        void registerCallbacks();
        void processSavePin();
        void processBeforeRestart();
        void processBeforeTablesUnload();
        void processInputKo(GroupObject& iKo);

#ifdef LOG_StartupDelayBase
        uint32_t startupDelay;
        bool processStartupDelay();
#endif
#ifdef LOG_HeartbeatDelayBase
        uint32_t heartbeatDelay;
        void processHeartbeat();
#endif
    };
} // namespace OpenKNX

extern OpenKNX::Common openknx;