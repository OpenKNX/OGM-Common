#pragma once
#include "OpenKNX/Base.h"

namespace OpenKNX
{
    class Channel : public Base
    {
      protected:
        uint8_t _channelIndex = 0;

      public:
        /*
         * The name of channel. Used for "log" methods.
         * @return name
         */
        virtual const char* name() override;

        /*
         * The channel index
         * @return channelIndex
         */
        virtual uint8_t channelIndex();
    };
} // namespace OpenKNX