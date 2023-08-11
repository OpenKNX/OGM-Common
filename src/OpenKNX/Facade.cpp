#include "OpenKNX/Facade.h"

extern void setup1() __attribute__((weak));

namespace OpenKNX
{

    void Facade::init(uint8_t firmwareRevision)
    {
        common.init(firmwareRevision);
    }
    void Facade::setup()
    {
        common.setup();
    }

    void Facade::loop()
    {
        common.loop();
    }

    bool Facade::usesDualCore()
    {
#ifdef OPENKNX_DUALCORE
        if (::setup1)
            return true;
#endif
        return false;
    }

#ifdef OPENKNX_DUALCORE
    void Facade::loop1()
    {
        common.loop1();
    }

    void Facade::setup1()
    {
        common.setup1();
    }
#endif

    bool Facade::afterStartupDelay()
    {
        return common.afterStartupDelay();
    }

    bool Facade::freeLoopTime()
    {
        return common.freeLoopTime();
    }

    void Facade::addModule(uint8_t id, Module *module)
    {
        modules.count++;
        modules.list[modules.count - 1] = module;
        modules.ids[modules.count - 1] = id;
#ifdef OPENKNX_RUNTIME_STAT
        modules.runtime[modules.count - 1] = RuntimeStat();
#endif
    }

    Modules *Facade::getModules()
    {
        return &modules;
    }

    Module *Facade::getModule(uint8_t id)
    {
        for (uint8_t i = 0; i < modules.count; i++)
        {
            if (modules.ids[i] == id)
                return modules.list[i];
        }

        return nullptr;
    }
} // namespace OpenKNX
OpenKNX::Facade openknx;