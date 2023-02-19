#pragma once
#include "OpenKNX/Base.h"

namespace OpenKNX
{
    class Channel : public Base
    {
      protected:
        uint8_t _channelIndex = 0;

        /*
         * Get a Pointer to a name for Log
         * THe point need to be delete[] after usage!
         */
        virtual const std::string logPrefix() override;

      public:
        /*
         * The channel index
         * @return channelIndex
         */
        virtual uint8_t channelIndex();
    };
} // namespace OpenKNX