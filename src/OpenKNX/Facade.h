#pragma once
#include "Helper.h"
#include "OpenKNX/Common.h"
#include "OpenKNX/Console.h"
#include "OpenKNX/Flash/Default.h"
#include "OpenKNX/Hardware.h"
#include "OpenKNX/Information.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Module.h"
#ifdef OPENKNX_RUNTIME_STAT
    #include "OpenKNX/Stat/RuntimeStat.h"
#endif
#include "OpenKNX/TimerInterrupt.h"
#include "OpenKNX/defines.h"

namespace OpenKNX
{
    struct Modules
    {
        uint8_t count = 0;
        uint8_t ids[OPENKNX_MAX_MODULES];
        Module* list[OPENKNX_MAX_MODULES];
#ifdef OPENKNX_RUNTIME_STAT
        // TODO check integration into Module
        Stat::RuntimeStat runtime[OPENKNX_MAX_MODULES];
    #ifdef OPENKNX_DUALCORE
        Stat::RuntimeStat runtime1[OPENKNX_MAX_MODULES];
    #endif
#endif
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
#ifdef INFO_LED_PIN
        Led infoLed;
#endif
#ifdef INFO1_LED_PIN
        Led info1Led;
#endif
#ifdef INFO2_LED_PIN
        Led info2Led;
#endif
        Modules modules;
        Flash::Driver openknxFlash;
        Flash::Driver knxFlash;

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
        bool freeLoopIterate(uint8_t size, uint8_t& position, uint8_t& processed);
    };
} // namespace OpenKNX

extern OpenKNX::Facade openknx;