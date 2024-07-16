#include "OpenKNX/Facade.h"
#include "OpenKNX/ChannelOwnerModule.h"

namespace OpenKNX
{
    ChannelOwnerModule::ChannelOwnerModule(uint8_t numberOfChannels)
        : _numberOfChannels(numberOfChannels)
    {
        if (_numberOfChannels > 0)
        {
            _pChannels = new OpenKNX::Channel*[numberOfChannels]();
        }
    }

    ChannelOwnerModule::~ChannelOwnerModule()
    {
        if (_pChannels != nullptr)
        {
            delete[] _pChannels;
            _pChannels = nullptr;
        }
    }

    OpenKNX::Channel* ChannelOwnerModule::createChannel(uint8_t _channelIndex /* this parameter is used in macros, do not rename */)
    {
        return nullptr;
    }

    void ChannelOwnerModule::setup()
    {
        OpenKNX::Module::setup();
        if (_pChannels != nullptr)
        {
            logDebugP("Setting up %d channels", _numberOfChannels);
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                logDebugP("Create channel %d", _channelIndex);
                logIndentUp();
                _pChannels[_channelIndex] = createChannel(_channelIndex);
                logIndentDown();
            }
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                {
                    logDebugP("Init channel %d", _channelIndex);
                    logIndentUp();
                    channel->init();
                    logIndentDown();

                    logDebugP("Setup channel %d - setup(true)", _channelIndex);
                    logIndentUp();
                    channel->setup(true);
                    logIndentDown();

                    logInfoP("Setup channel %d - setup()", _channelIndex);
                    logIndentUp();
                    channel->setup();
                    logIndentDown();
                }
            }
        }
    }

    void ChannelOwnerModule::loop(bool configured)
    {
        OpenKNX::Module::loop(configured);
        if (_pChannels != nullptr)
        {
            uint8_t processed = 0;
            do
            {
                OpenKNX::Channel* channel = _pChannels[_currentChannel];
                if (channel != nullptr)
                {
                    channel->loop(configured);
                }
            }
            while (openknx.freeLoopIterate(_numberOfChannels, _currentChannel, processed));
        }
    }

    OpenKNX::Channel* ChannelOwnerModule::getChannel(uint8_t channelIndex)
    {
        return _pChannels != nullptr ? _pChannels[channelIndex] : nullptr;
    }

    uint8_t ChannelOwnerModule::getNumberOfChannels()
    {
        return _numberOfChannels;
    }

    uint8_t ChannelOwnerModule::getNumberOfUsedChannels()
    {
        uint8_t activeChannels = 0;
        if (_pChannels != nullptr)
        {
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                    activeChannels++;
            }
        }
        return activeChannels;
    }

    void ChannelOwnerModule::loop()
    {
        OpenKNX::Module::loop();
        if (_pChannels != nullptr)
        {
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                    channel->loop();
            }
        }
    }

#ifdef OPENKNX_DUALCORE
    void ChannelOwnerModule::ChannelOwnerModule::setup1(bool configured)
    {
        OpenKNX::Module::setup1(configured);
        if (_pChannels != nullptr)
        {
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                {
                    channel->setup1(configured);
                }
            }
        }
    }

    void ChannelOwnerModule::ChannelOwnerModule::setup1()
    {
        OpenKNX::Module::setup1();
        if (_pChannels != nullptr)
        {
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                {
                    channel->setup1();
                }
            }
        }
    }
    /*
     * Module logic for core1
     */
    void ChannelOwnerModule::loop1(bool configured)
    {
        OpenKNX::Module::loop1(configured);
        if (_pChannels != nullptr)
        {
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                {
                    channel->loop1(configured);
                }
            }
        }
    }
    void ChannelOwnerModule::loop1()
    {
        OpenKNX::Module::loop1();
        if (_pChannels != nullptr)
        {
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                {
                    channel->loop1();
                }
            }
        }
    }
#endif // OPENKNX_DUALCORE

#if (MASK_VERSION & 0x0900) != 0x0900 // Coupler do not have GroupObjects
    /*
     * Called on incoming/changing GroupObject
     * @param GroupObject
     */
    void ChannelOwnerModule::processInputKo(GroupObject& ko)
    {
        OpenKNX::Module::processInputKo(ko);
        if (_pChannels != nullptr)
        {
            for (uint8_t _channelIndex = 0; _channelIndex < _numberOfChannels; _channelIndex++)
            {
                OpenKNX::Channel* channel = _pChannels[_channelIndex];
                if (channel != nullptr)
                {
                    channel->processInputKo(ko);
                }
            }
        }
    }
#endif // MASK_VERION

} // namespace OpenKNX