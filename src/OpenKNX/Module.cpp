#include "OpenKNX/Module.h"
#include "OpenKNX/Common.h"
#include <knx.h>

namespace OpenKNX
{
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
        return "UnnamedModule";
    }

    const char *Module::version()
    {
        return "0";
    }

    void Module::log(const char *output, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, output);
        vsnprintf(buffer, 256, output, args);
        va_end(args);
        openknx.log(name(), buffer);
    }

    void Module::logHex(const uint8_t* data, size_t size)
    {
        openknx.logHex(name(), data, size);
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
