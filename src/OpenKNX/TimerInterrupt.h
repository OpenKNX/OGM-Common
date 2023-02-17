#pragma once
#include "knx.h"

#ifndef OPENKNX_INTERRUPT_TIMER_MS
#define OPENKNX_INTERRUPT_TIMER_MS 5
#endif

namespace OpenKNX
{
    // IMPORTANT!!! The method millis() and micros() are not incremented further in the interrupt!
    class TimerInterrupt
    {
      public:
        void init();
        void interrupt();
    };
} // namespace OpenKNX