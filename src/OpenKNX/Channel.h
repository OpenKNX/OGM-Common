#pragma once
#include "OpenKNX/Module.h"

namespace OpenKNX
{
    class Channel : public Module
    {
      protected:
        uint8_t _channelIndex = 0;

      public:
        virtual const char* name();
      
    };
} // namespace OpenKNX