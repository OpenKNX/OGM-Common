#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"
#include <knx.h>

namespace OpenKNX
{
    uint8_t Channel::channelIndex()
    {
        return _channelIndex;
    }

    char *Channel::logName()
    {
        char *buffer = new char[MAX_LOG_PREFIX + 1];
        sprintf(buffer, "%s<%i>", name(), _channelIndex + 1);
        return buffer;
    }
} // namespace OpenKNX
