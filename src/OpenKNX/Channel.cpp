#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"
#include <knx.h>

namespace OpenKNX
{
    uint8_t Channel::channelIndex()
    {
        return _channelIndex;
    }

    const std::string Channel::logPrefix()
    {
        return openknx.logger.logPrefix(name(), _channelIndex + 1);
    }
} // namespace OpenKNX
