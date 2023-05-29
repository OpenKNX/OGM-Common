#include "OpenKNX/Base.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    const std::string Base::logPrefix()
    {
        return name();
    }

    const std::string Base::name()
    {
        return "Unnamed";
    }

    void Base::loop()
    {}

    void Base::loop1()
    {}

    void Base::init()
    {}

    void Base::setup()
    {}
#if (MASK_VERSION & 0x0900) != 0x0900   // Coupler do not have GroupObjects
    void Base::processInputKo(GroupObject &ko)
    {}
#endif
    bool Base::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        return false;
    }
    
    bool Base::processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
    {
        return false;
    }
    bool Base::processDiagnoseCommand(const char *input, char *output, uint8_t line)
    {
        return false;
    }

} // namespace OpenKNX
