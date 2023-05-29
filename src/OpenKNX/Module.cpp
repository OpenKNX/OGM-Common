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

    bool Module::usesDualCore()
    {
        return false;
    }

    bool Module::HasInformations()
    {
        return false;
    }

    void Module::showInformations()
    {
        // Example usage:
        //openknx.logger.log("Some-Info:", _myInfo);
        //openknx.logger.log("Another-Info:", _someOtherInfo);
    }

    void Module::processSerialInput(uint8_t command)
    {
        // Example usage:
        // switch(command)
        // {
        //     case 'x':
        //         logInfoP("Test123");
        //     break;
        // }
    }

    void Module::showHelp()
    {
        // Example usage:
        //openknx.logger.log("", ">  x  <  Foo [%s]", name().c_str());
    }

} // namespace OpenKNX
