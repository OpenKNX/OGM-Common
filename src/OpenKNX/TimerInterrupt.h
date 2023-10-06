#pragma once
#include "defines.h"
#include <Arduino.h>

// Interval of interrupt for leds and free memory collector
#define OPENKNX_INTERRUPT_TIMER_MS 3

namespace OpenKNX
{
    // IMPORTANT!!! The method millis() and micros() are not incremented further in the interrupt!
    class TimerInterrupt
    {
      private:
        volatile uint32_t _time = 0;
#ifdef OPENKNX_DUALCORE
        volatile uint32_t _time1 = 0;
#endif

#ifdef ARDUINO_ARCH_RP2040
        struct repeating_timer _repeatingTimer;
        alarm_pool_t *_alarmPool;
    #ifdef OPENKNX_DUALCORE
        struct repeating_timer _repeatingTimer1;
        alarm_pool_t *_alarmPool1;
    #endif
#endif

      public:
        void init();
        void interrupt();
#ifdef ARDUINO_ARCH_RP2040
        alarm_pool_t *repeatingTimer();
#endif

#ifdef OPENKNX_DUALCORE
        void init1();
        void interrupt1();
    #ifdef ARDUINO_ARCH_RP2040
        alarm_pool_t *repeatingTimer1();
    #endif
#endif
    };
} // namespace OpenKNX