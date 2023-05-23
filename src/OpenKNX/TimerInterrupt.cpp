#include "OpenKNX/TimerInterrupt.h"
#include "OpenKNX/Common.h"

/*
 * Select a Interrupt for global TimerInterrupt
 * OPENKNX_TIMER_INTERRUPT
 *
 * RP2040 4 Timers available
 * 0 = TIMER_IRQ0
 * 1 = TIMER_IRQ1 (default)
 * 2 = TIMER_IRQ2
 * 3 = TIMER_IRQ3
 * *
 * SAMD 6 Timer available
 * 0 = TIMER_TC3
 * 1 = TIMER_TC4 (default)
 * 2 = TIMER_TC5
 * 3 = TIMER_TCC
 * 4 = TIMER_TCC1
 * 5 = TIMER_TCC2
 */
#ifndef OPENKNX_TIMER_INTERRUPT
#define OPENKNX_TIMER_INTERRUPT 1
#endif

#undef SELECTED_TIMER

#ifdef ARDUINO_ARCH_RP2040
#if OPENKNX_TIMER_INTERRUPT >= 0 && OPENKNX_TIMER_INTERRUPT <= 3
#define SELECTED_TIMER OPENKNX_TIMER_INTERRUPT
#endif
#endif

#ifdef ARDUINO_ARCH_SAMD
#if OPENKNX_TIMER_INTERRUPT == 0
#define SELECTED_TIMER TIMER_TC3
#undef USING_TIMER_TC3
#define USING_TIMER_TC3 true
#endif
#if OPENKNX_TIMER_INTERRUPT == 1
#define SELECTED_TIMER TIMER_TC4
#undef USING_TIMER_TC4
#define USING_TIMER_TC4 true
#endif
#if OPENKNX_TIMER_INTERRUPT == 2
#define SELECTED_TIMER TIMER_TC5
#undef USING_TIMER_TC5
#define USING_TIMER_TC5 true
#endif
#if OPENKNX_TIMER_INTERRUPT == 3
#define SELECTED_TIMER TIMER_TCC
#undef USING_TIMER_TCC
#define USING_TIMER_TCC true
#endif
#if OPENKNX_TIMER_INTERRUPT == 4
#define SELECTED_TIMER TIMER_TCC1
#undef USING_TIMER_TCC1
#define USING_TIMER_TCC1 true
#endif
#if OPENKNX_TIMER_INTERRUPT == 5
#define SELECTED_TIMER TIMER_TCC2
#undef USING_TIMER_TCC2
#define USING_TIMER_TCC2 true
#endif
#endif

#ifndef SELECTED_TIMER
#error Invalid OPENKNX_TIMER_INTERRUPT defined
#endif

// include after defines!
#include "TimerInterrupt_Generic.h"

// Select Timer Interrupt
#if defined(ARDUINO_ARCH_SAMD)
SAMDTimer ITimer(SELECTED_TIMER);
#elif defined(ARDUINO_ARCH_RP2040)
RPI_PICO_Timer ITimer(SELECTED_TIMER);
#endif

namespace OpenKNX
{
    void TimerInterrupt::init()
    {
        // Register 1ms
#if defined(ARDUINO_ARCH_SAMD)
        ITimer.attachInterruptInterval_MS(OPENKNX_INTERRUPT_TIMER_MS, []() -> void {
            openknx.timerInterrupt.interrupt();
        });
#elif defined(ARDUINO_ARCH_RP2040)
        ITimer.attachInterrupt(OPENKNX_INTERRUPT_TIMER_MS * 1000, [](repeating_timer *t) -> bool {
            // digitalWrite(3, HIGH);
            openknx.timerInterrupt.interrupt();
            // digitalWrite(3, LOW);
            return true;
        });
#endif
    }

#ifdef __time_critical_func
    void __isr __time_critical_func(TimerInterrupt::interrupt)()
#else
    void TimerInterrupt::interrupt()
#endif
    {
        switch (_counter % 3)
        {
            case 0:
                // collect memory usage
                openknx.collectMemoryStats();
                break;
            case 1:
                // loop progLed
                openknx.hardware.progLed.loop();
                break;
            case 2:
                // loop infoLed
                openknx.hardware.infoLed.loop();
                break;

            default:
                break;
        }

        _counter++;
    }

} // namespace OpenKNX