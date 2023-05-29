#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    void Facade::init(uint8_t firmwareRevision)
    {
        common.init(firmwareRevision);
    }

    void Facade::loop()
    {
        common.loop();
    }

    void Facade::setup()
    {
        common.setup();
    }

    void Facade::loop1()
    {
        common.loop1();
    }

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
        openknx.modules.count++;
        openknx.modules.list[openknx.modules.count - 1] = module;
        openknx.modules.ids[openknx.modules.count - 1] = id;
    }

    Modules* Facade::getModules()
    {
        return &modules;
    }

    Module *Facade::getModule(uint8_t id)
    {
        for (uint8_t i = 0; i < openknx.modules.count; i++)
        {
            if (openknx.modules.ids[i] == id)
                return openknx.modules.list[i];
        }

        return nullptr;
    }
} // namespace OpenKNX
OpenKNX::Facade openknx;