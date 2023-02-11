#include "OpenKNX/Base.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    char *Base::logName()
    {
        char *buffer = new char[MAX_LOG_PREFIX + 1];
        sprintf(buffer, "%s", name());
        return buffer;
    }

    void Base::log(const char *output, ...)
    {
        char* name = logName();
        char buffer[256];
        va_list args;
        va_start(args, output);
        vsnprintf(buffer, 256, output, args);
        va_end(args);
        openknx.log(name, buffer);
        delete[] name;
    }

    void Base::logHex(const uint8_t *data, size_t size)
    {
        char* name = logName();
        openknx.logHex(name, data, size);
        delete[] name;
    }

    const char *Base::name()
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
