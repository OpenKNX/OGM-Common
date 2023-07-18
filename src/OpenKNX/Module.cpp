#include "OpenKNX/Module.h"
#include "OpenKNX/Facade.h"

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

    bool Module::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        return false;
    }
    bool Module::processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        return false;
    }

    void Module::savePower()
    {}

    bool Module::restorePower()
    {
        return true;
    }

    void Module::showInformations()
    {
        // Example usage:
        // logInfoP("Some-String: %s", myString);
        // logInfoP("Some-Integer: %i", myInteger);
    }

    bool Module::processCommand(const std::string cmd, bool diagnoseKo)
    {
        return false;
    }

    void Module::showHelp()
    {
        // Example usage:
        // openknx.console.printHelpLine("command 123", "the description");
    }

} // namespace OpenKNX
