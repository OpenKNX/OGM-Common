#pragma once
#include "Helper.h"
#include "OpenKNX/Common.h"
#include "OpenKNX/Console.h"
#include "OpenKNX/Flash/Default.h"
#include "OpenKNX/GPIO/GPIO.h"
#include "OpenKNX/Hardware.h"
#include "OpenKNX/Information.h"
#include "OpenKNX/Led/FunctionManager.h"
#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/Module.h"
#include "OpenKNX/Time/Calendar.h"
#include "OpenKNX/Time/TimeManager.h"
#include "OpenKNX/Time/TimeProvider.h"
#include "OpenKNX/Watchdog.h"

#ifdef ParamBASE_Latitude
    #include "OpenKNX/Sun/SunCalculation.h"
#endif

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
        Watchdog watchdog;
        Time::TimeManager time;
#ifdef ParamBASE_Latitude
        Sun::SunCalculation sun;
#endif
        Time::Calendar calendar;

        Button progButton = Button("Prog");
#ifdef FUNC1_BUTTON_PIN
        Button func1Button = Button("Func1");
#endif
#ifdef FUNC2_BUTTON_PIN
        Button func2Button = Button("Func2");
#endif
#ifdef FUNC3_BUTTON_PIN
        Button func3Button = Button("Func3");
#endif

        Led::Manager leds;
        Led::FunctionManager ledFunctions;

        Modules modules;
        Flash::Driver openknxFlash;
        Flash::Driver knxFlash;
        GPIO::Manager gpio;

#ifdef FIRMWARE_REVISION
        void init();
#else
        void init(uint8_t firmwareRevision);
#endif
        void loop();
        void setup();
        bool usesDualCore();
#ifdef OPENKNX_DUALCORE
        void loop1();
        void setup1();
#endif
        void addModule(uint8_t id, Module& module);
        Module* getModule(uint8_t id);
        Modules* getModules();
        bool afterStartupDelay();
        bool freeLoopTime();
        bool freeLoopIterate(uint8_t size, uint8_t& position, uint8_t& processed);
        void restart();
        void unsupportedEtsModule(uint8_t etsModuleId);
    };
} // namespace OpenKNX

extern OpenKNX::Facade openknx;