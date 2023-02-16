#include "OpenKNX/TimerInterrupt.h"
#include "OpenKNX/Common.h"

#if defined(ARDUINO_ARCH_SAMD)

#define USING_TIMER_TC3 false
#define USING_TIMER_TC4 true
#define USING_TIMER_TC5 false
#define USING_TIMER_TCC false
#define USING_TIMER_TCC1 false
#define USING_TIMER_TCC2 false

#if USING_TIMER_TC3
#define SELECTED_TIMER TIMER_TC3
#elif USING_TIMER_TC4
#define SELECTED_TIMER TIMER_TC4
#elif USING_TIMER_TC5
#define SELECTED_TIMER TIMER_TC5
#elif USING_TIMER_TCC
#define SELECTED_TIMER TIMER_TCC
#elif USING_TIMER_TCC1
#define SELECTED_TIMER TIMER_TCC1
#elif USING_TIMER_TCC2
#define SELECTED_TIMER TIMER_TCC2
#else
#error You have to select 1 Timer
#endif

#endif

// include after defines!
#include "ISR_Timer_Generic.h"
#include "TimerInterrupt_Generic.h"

#if defined(ARDUINO_ARCH_SAMD)
SAMDTimer ITimer(SELECTED_TIMER);
#elif defined(ARDUINO_ARCH_RP2040)
RPI_PICO_Timer ITimer(1);
#endif
ISR_Timer ISRTimer;

namespace OpenKNX
{
    void TimerInterrupt::init()
    {
        // Register 1ms
#if defined(ARDUINO_ARCH_SAMD)
        ITimer.attachInterruptInterval_MS(OPENKNX_INTERRUPT_TIMER_MS, []() -> void {
            // openknx.timerInterrupt.interrupt();
            ISRTimer.run();
        });
#elif defined(ARDUINO_ARCH_RP2040)
        ITimer.attachInterrupt(OPENKNX_INTERRUPT_TIMER_MS * 1000, [](repeating_timer *t) -> bool {
            // openknx.timerInterrupt.interrupt();
            ISRTimer.run();
            return true;
        });
#endif

        ISRTimer.setInterval(1, []() -> void {
            openknx.collectMemoryStats();
        });

        ISRTimer.setInterval(5, []() -> void {
            openknx.hardware.progLed.loop();
            openknx.hardware.infoLed.loop();
        });
    }

    // Alternative for ISR_Timer
    void TimerInterrupt::interrupt()
    {
        
        // collect memory usage
        openknx.collectMemoryStats();

        // loop prog & info led
        openknx.hardware.progLed.loop();
        openknx.hardware.infoLed.loop();
    }

} // namespace OpenKNX