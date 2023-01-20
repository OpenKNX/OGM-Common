#include <knx.h>
#include "OpenKNX/Channel.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    uint32_t Channel::calcParamIndex(uint16_t paramIndex)
    {
        return paramIndex + (_channelIndex * _channelParamBlockSize) + _channelParamOffset;
    }

    uint16_t Channel::calcKoNumber(uint8_t koIndex)
    {
        return koIndex + (_channelIndex * _channelParamKoBlockSize) + _channelParamKoOffset;
    }

    int8_t Channel::calcKoIndex(uint16_t koNumber)
    {
        int16_t result = (koNumber - _channelParamKoOffset);
        // check if channel is valid
        if ((int8_t)(result / _channelParamKoBlockSize) == _channelIndex)
            result = result % _channelParamKoBlockSize;
        else
            result = -1;
        return (int8_t)result;
    }

    GroupObject *Channel::getKo(uint8_t koIndex)
    {
        return &knx.getGroupObject(calcKoNumber(koIndex));
    }

    const char *Channel::name()
    {
        return Module::name();
        //int status = -4; // some arbitrary value to eliminate the compiler warning
        //return abi::__cxa_demangle(typeid(*this).name(), NULL, NULL, &status);
    }
} // namespace OpenKNX
