#include "OpenKNX/Helper.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Helper::log(const std::string message)
    {
        openknx.logger.log("Deprecated", "use logXXXX()");
        openknx.logger.log(message);
    }

    void Helper::log(const std::string prefix, const std::string message, ...)
    {
        va_list args;
        va_start(args, message);
        openknx.logger.log("Deprecated", "use logXXXX()");
        openknx.logger.log(prefix, message, args);
        va_end(args);
    }

    void Helper::logHex(const std::string prefix, const uint8_t* data, size_t size)
    {
        openknx.logger.log("Deprecated", "use logHexXXXX()");
        openknx.logger.logHex(prefix, data, size);
    }
} // namespace OpenKNX
