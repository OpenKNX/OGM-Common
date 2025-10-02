#include "OpenKNX/Facade.h"
#include "ModuleVersionCheck.h"

extern void setup1() __attribute__((weak));

namespace OpenKNX
{

#ifdef FIRMWARE_REVISION
    void Facade::init()
    {
        common.init(FIRMWARE_REVISION);
    }
#else
    void Facade::init(uint8_t firmwareRevision)
    {
        common.init(firmwareRevision);
    }
#endif
    void Facade::setup()
    {
        common.setup();
    }

    void Facade::loop()
    {
        common.loop();
    }

    void Facade::restart()
    {
        common.restart();
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

    bool Facade::freeLoopIterate(uint8_t size, uint8_t &position, uint8_t &processed)
    {
        return common.freeLoopIterate(size, position, processed);
    }

    void Facade::addModule(uint8_t id, Module &module)
    {
        modules.count++;
        modules.list[modules.count - 1] = &module;
        modules.ids[modules.count - 1] = id;
#ifdef OPENKNX_RUNTIME_STAT
        modules.runtime[modules.count - 1] = Stat::RuntimeStat();
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

    /*
     * Declare a ets module as unsupported. The ets can then automatically disable the unsupported modules via a manual sync.
     * Allowed ets module id from 2 to 33 (BASE is not allowed to disabled so we start with 2)
     */
    void Facade::unsupportedEtsModule(uint8_t etsModuleId)
    {
        common.unsupportedEtsModule(etsModuleId);
    }
} // namespace OpenKNX
OpenKNX::Facade openknx;