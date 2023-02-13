#pragma once
#include "knx.h"

#ifndef OPENKNX_INTERRUPT_TIMER
#define OPENKNX_INTERRUPT_TIMER 1000
#endif

// #ifndef OPENKNX_COLLECT_MEMORY_INTERVAL
// #define OPENKNX_COLLECT_MEMORY_INTERVAL 1000
// #endif

namespace OpenKNX
{
    class TimerInterrupt
    {
      public:
        void interrupt();
        void init();
    };
} // namespace OpenKNX