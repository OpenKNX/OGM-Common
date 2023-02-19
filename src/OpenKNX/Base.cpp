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
        va_list args;
        va_start(args, message);
        openknx.logger.log(logPrefix(), message, args);
        va_end(args);
    }

    void Base::logHex(const uint8_t *data, size_t size)
    {
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
} // namespace OpenKNX
