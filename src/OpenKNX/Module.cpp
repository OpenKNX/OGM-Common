#include <knx.h>
#include <iostream>
#include <iomanip>
#include <cxxabi.h>
#include "OpenKNX/Common.h"
#include "OpenKNX/Module.h"

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
        int status = -4; // some arbitrary value to eliminate the compiler warning
        return abi::__cxa_demangle(typeid(*this).name(), NULL, NULL, &status);
    }
    
    const char *Module::version()
    {
        return "0";
    }

    int Module::debug(const char *output, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, output);
        int result = vsnprintf(buffer, 256, output, args);
        va_end(args);
        openknx.debug(name(), buffer);
        return result;
    }
} // namespace OpenKNX
