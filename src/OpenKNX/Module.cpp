#include "OpenKNX/Module.h"
#include "OpenKNX/Common.h"
#include <knx.h>

namespace OpenKNX
{
    const std::string Module::version()
    {
        return "0.0";
    }

    uint16_t Module::flashSize()
    {
        return 0;
    }

    void Module::writeFlash()
    {}

    void Module::readFlash(const uint8_t *data, const uint16_t size)
    {}

    void Module::processAfterStartupDelay()
    {}

    void Module::processBeforeRestart()
    {}

    void Module::processBeforeTablesUnload()
    {}

#ifdef USE_FUNCTIONPROPERTYCALLBACK
    bool Module::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        return false;
    }
#endif

    void Module::savePower()
    {}

    bool Module::restorePower()
    {
        return true;
    }

    bool Module::usesDualCore()
    {
        return false;
    }

} // namespace OpenKNX
