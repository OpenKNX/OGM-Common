#include "OpenKNX/TimerInterrupt.h"
#include "OpenKNX/Common.h"

// #include "ISR_Timer_Generic.h"
#include "TimerInterrupt_Generic.h"

#if defined(ARDUINO_ARCH_SAMD)
SAMDTimer ITimer(TIMER_TC5);
#elif defined(ARDUINO_ARCH_RP2040)
RPI_PICO_Timer ITimer(1);
#endif

namespace OpenKNX
{
    TimerInterrupt::TimerInterrupt()
    {
#if defined(ARDUINO_ARCH_SAMD)
        // ITimer.attachInterruptInterval_MS(OPENKNX_INTERUPPT_TIMER, []() -> void {
        //     openknx.timer_interrupt.interrupt();
        // });
#elif defined(ARDUINO_ARCH_RP2040)
        ITimer.attachInterrupt(OPENKNX_INTERUPPT_TIMER * 1000, [](repeating_timer *t) -> bool {
            openknx.timer_interrupt.interrupt();
            return true;
        });
#endif
    }

    void TimerInterrupt::interrupt()
    {
        openknx.collectMemoryStats();
    }

} // namespace OpenKNX