#include "OpenKNX/Base.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    char *Base::logPrefix()
    {
        char *buffer = new char[OPENKNX_MAX_LOG_PREFIX_LENGTH];
        sprintf(buffer, "%s", name());
        return buffer;
    }

    void Base::log(const char *message, ...)
    {
        char *prefix = logPrefix();
        va_list args;
        va_start(args, message);
        openknx.logger.log(LogLevel::Info, prefix, message, args);
        va_end(args);
        delete[] prefix;
    }

    void Base::log(LogLevel level, const char *message, ...)
    {
        char *prefix = logPrefix();
        va_list args;
        va_start(args, message);
        openknx.logger.log(level, prefix, message, args);
        va_end(args);
        delete[] prefix;
    }

    void Base::logHex(const uint8_t *data, size_t size)
    {
        char *prefix = logPrefix();
        openknx.logHex(prefix, data, size);
        delete[] prefix;
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
