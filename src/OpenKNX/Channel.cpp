#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"
#include <knx.h>

namespace OpenKNX
{
    const char *Channel::name()
    {
        return appendChannelSuffix("Unnamed");
    }

    uint8_t Channel::channelIndex()
    {
        return _channelIndex;
    }

    const char *Channel::appendChannelSuffix(const char *name)
    {
        char *buffer = new char[MAX_LOG_PREFIX];
        sprintf(buffer, "%s<%i>", name, _channelIndex + 1);
        return clone_const_chars(buffer);
    }
} // namespace OpenKNX
