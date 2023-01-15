#include "OpenKNX.h"

namespace OpenKNX
{
    uint32_t Module::calcParamIndex(uint16_t iParamIndex)
    {
        return iParamIndex + (mChannelIndex * mChannelParamBlockSize) + mChannelParamOffset;
    }

    uint16_t Module::calcKoNumber(uint8_t iKoIndex)
    {
        return iKoIndex + (mChannelIndex * mChannelParamKoBlockSize) + mChannelParamKoOffset;
    }

    int8_t Module::calcKoIndex(uint16_t iKoNumber)
    {
        int16_t lResult = (iKoNumber - mChannelParamKoOffset);
        // check if channel is valid
        if ((int8_t)(lResult / mChannelParamKoBlockSize) == mChannelIndex)
            lResult = lResult % mChannelParamKoBlockSize;
        else
            lResult = -1;
        return (int8_t)lResult;
    }

    GroupObject *Module::getKo(uint8_t iKoIndex)
    {
        return &knx.getGroupObject(calcKoNumber(iKoIndex));
    }

    void Module::loop()
    {}

    void Module::setup()
    {}
    void Module::firstLoop()
    {}

    void Module::processInputKo(GroupObject &iKo)
    {}

    void Module::processSavePin()
    {}

    void Module::processBeforeRestart()
    {}

    void Module::processBeforeTablesUnload()
    {}

    const char *Module::name()
    {
        int status = -4; // some arbitrary value to eliminate the compiler warning
        return abi::__cxa_demangle(typeid(*this).name(), NULL, NULL, &status);
    }

    int Module::log(const char *output, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, output);
        int result = vsnprintf(buffer, 256, output, args);
        va_end(args);
        openknx.log(name(), buffer);
        return result;
    }
} // namespace OpenKNX
