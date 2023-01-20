#include "OpenKNX/Module.h"
#include "OpenKNX/Common.h"
#include <cxxabi.h>
#include <iomanip>
#include <iostream>
#include <knx.h>

namespace OpenKNX
{
    GroupObject *Module::getKo(uint16_t koNumber)
    {
        return &knx.getGroupObject(koNumber);
    }

    void Module::loop()
    {}

    void Module::setup()
    {}

    void Module::firstLoop()
    {}

    void Module::processInputKo(GroupObject &ko)
    {}

    void Module::processSavePin()
    {}

    void Module::processBeforeRestart()
    {}

    void Module::processBeforeTablesUnload()
    {}

    const char *Module::name()
    {
        return "UnknownModule";
    }

    const char *Module::version()
    {
        return "0";
    }

    void Module::debug(const char *output, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, output);
        vsnprintf(buffer, 256, output, args);
        va_end(args);
        openknx.debug(name(), buffer);
    }

    void Module::writeFlash()
    {}

    void Module::readFlash(const uint8_t *data, const uint16_t size)
    {}

    uint16_t Module::flashSize()
    {
        return 0;
    }
} // namespace OpenKNX
