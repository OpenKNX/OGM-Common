#include "OpenKNX/Base.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    const std::string Base::logPrefix()
    {
        return name();
    }

    void Base::log(const char *message, ...)
    {
        logError("The direct call via log is deprecated");
        va_list args;
        va_start(args, message);
        openknx.logger.log(logPrefix(), message, args);
        va_end(args);
    }

    void Base::logHex(const uint8_t *data, size_t size)
    {
        logError("The direct call via log is deprecated");
        openknx.logHex(logPrefix(), data, size);
    }

    const std::string Base::name()
    {
        return "Unnamed";
    }

    void Base::loop()
    {}

    void Base::loop2()
    {}

    void Base::setup()
    {}

    void Base::processInputKo(GroupObject &ko)
    {}

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
