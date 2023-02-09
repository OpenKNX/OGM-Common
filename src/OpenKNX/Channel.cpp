#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"
#include <knx.h>

namespace OpenKNX
{
    const char *Channel::name()
    {
        return "UnnamedChannel";
    }

    uint8_t Channel::channelIndex()
    {
        return _channelIndex;
    }
} // namespace OpenKNX
