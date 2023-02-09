#include "OpenKNX/Base.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Base::log(const char *output, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, output);
        vsnprintf(buffer, 256, output, args);
        va_end(args);
        openknx.log(name(), buffer);
    }

    void Base::logHex(const uint8_t *data, size_t size)
    {
        openknx.logHex(name(), data, size);
    }

    const char *Base::name()
    {
        return "Unnamed";
    }

    void Base::loop()
    {}

    void Base::setup()
    {}

    void Base::processInputKo(GroupObject &ko)
    {}
} // namespace OpenKNX
