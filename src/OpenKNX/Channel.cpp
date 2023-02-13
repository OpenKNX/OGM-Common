#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"
#include <knx.h>

namespace OpenKNX
{
    uint8_t Channel::channelIndex()
    {
        return _channelIndex;
    }

    char *Channel::logPrefix()
    {
        char *buffer = new char[OPENKNX_MAX_LOG_PREFIX_LENGTH];
        sprintf(buffer, "%s<%i>", name(), _channelIndex + 1);
        return buffer;
    }
} // namespace OpenKNX
