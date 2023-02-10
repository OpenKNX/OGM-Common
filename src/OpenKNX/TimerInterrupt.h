#pragma once
#include "knx.h"

#ifndef OPENKNX_INTERUPPT_TIMER
#define OPENKNX_INTERUPPT_TIMER 1
#endif

namespace OpenKNX
{
    class TimerInterrupt
    {
      public:
        TimerInterrupt();
        void interrupt();
    };
} // namespace OpenKNX