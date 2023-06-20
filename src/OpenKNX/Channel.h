#pragma once
#include "OpenKNX/Base.h"
#include <string>

namespace OpenKNX
{
    class Channel : public Base
    {
      protected:
        uint8_t _channelIndex = 0;

        /*
         * Build prefix for a Channel. Format is: ChannelName<ChannelIndex>
         *
         * @return prefix
         */
        virtual const std::string logPrefix() override;

      public:
        /*
         * The channel index
         *
         * @return channelIndex
         */
        virtual uint8_t channelIndex();
    };
} // namespace OpenKNX