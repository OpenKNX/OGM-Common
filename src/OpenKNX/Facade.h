#pragma once
#include "Helper.h"
#include "OpenKNX/Common.h"
#include "OpenKNX/Console.h"
#include "OpenKNX/Flash/Default.h"
#include "OpenKNX/Hardware.h"
#include "OpenKNX/Information.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Module.h"
#include "OpenKNX/TimerInterrupt.h"
#include "OpenKNX/defines.h"

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
        Facade();
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
        Flash::Driver flashOpenKNX;
        Flash::Driver flashKNX;
        volatile bool ready = false;

        void init(uint8_t firmwareRevision);
        void loop();
        void setup();
        bool usesDualCore();
#ifdef OPENKNX_DUALCORE
        void loop1();
        void setup1();
#endif
        void addModule(uint8_t id, Module* module);
        Module* getModule(uint8_t id);
        Modules* getModules();
        bool afterStartupDelay();
        bool freeLoopTime();
    };
} // namespace OpenKNX

extern OpenKNX::Facade openknx;