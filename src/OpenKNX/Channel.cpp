#include "OpenKNX/Channel.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    uint8_t Channel::channelIndex()
    {
        return _channelIndex;
    }

    const std::string Channel::logPrefix()
    {
        return openknx.logger.buildPrefix(name(), _channelIndex + 1);
    }
} // namespace OpenKNX
