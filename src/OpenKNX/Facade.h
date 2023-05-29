#pragma once
#include "OpenKNX/defines.h"
#include "OpenKNX/Common.h"
#include "OpenKNX/Console.h"
#include "OpenKNX/Flash/Default.h"
#include "OpenKNX/Hardware.h"
#include "OpenKNX/Information.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Module.h"
#include "OpenKNX/TimerInterrupt.h"
#include "../Helper.h"

namespace OpenKNX
{
    struct Modules
    {
        uint8_t count = 0;
        uint8_t ids[OPENKNX_MAX_MODULES];
        Module* list[OPENKNX_MAX_MODULES];
    };

    class Facade
    {
      public:
        Common common;
        Flash::Default flash;
        Information info;
        Console console;
        Log::Logger logger;
        TimerInterrupt timerInterrupt;
        Hardware hardware;
        Led progLed;
        Led infoLed;
        Modules modules;

        void init(uint8_t firmwareRevision);
        void loop();
        void setup();
        void loop1();
        void addModule(uint8_t id, Module* module);
        Module* getModule(uint8_t id);
        Modules* getModules();
        bool afterStartupDelay();
        bool freeLoopTime();
    };
} // namespace OpenKNX

extern OpenKNX::Facade openknx;