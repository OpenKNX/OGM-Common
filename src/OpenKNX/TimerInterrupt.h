#pragma once
#include "defines.h"
#include "knx.h"

// Interval of interrupt for leds and free memory collector
#define OPENKNX_INTERRUPT_TIMER_MS 2

namespace OpenKNX
{
    // IMPORTANT!!! The method millis() and micros() are not incremented further in the interrupt!
    class TimerInterrupt
    {
      private:
        volatile uint8_t _counter = 0;
      public:
        void init();
        void interrupt();
    };
} // namespace OpenKNX