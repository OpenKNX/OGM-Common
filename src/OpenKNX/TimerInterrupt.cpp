#include "OpenKNX/TimerInterrupt.h"
#include "OpenKNX/Facade.h"

#ifndef ARDUINO_ARCH_RP2040
    /*
     * Select a Interrupt for global TimerInterrupt
     * OPENKNX_TIMER_INTERRUPT
     * *
     * SAMD 6 Timer available
     * 0 = TIMER_TC3
     * 1 = TIMER_TC4 (default)
     * 2 = TIMER_TC5
     * 3 = TIMER_TCC
     * 4 = TIMER_TCC1
     * 5 = TIMER_TCC2
     *
     * ESP32 4 Timers available
     * 0 = Timer0
     * 1 = Timer1 (default)
     * 2 = Timer2
     * 3 = Timer3
     * *
     */
    #ifndef OPENKNX_TIMER_INTERRUPT
        #define OPENKNX_TIMER_INTERRUPT 1
    #endif

    #undef SELECTED_TIMER

    #ifdef ARDUINO_ARCH_ESP32
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
    #elif defined(ARDUINO_ARCH_ESP32)
ESP32Timer ITimer(SELECTED_TIMER);
    #endif

#endif

#ifdef ARDUINO_ARCH_RP2040
bool __isr __time_critical_func(timerInterruptCallback)(repeating_timer *t)
{
    openknx.timerInterrupt.interrupt();
    return true;
}
    #ifdef OPENKNX_DUALCORE
        #ifdef ARDUINO_ARCH_RP2040
bool __isr __time_critical_func(timerInterruptCallback1)(repeating_timer *t)
{
    openknx.timerInterrupt.interrupt1();
    return true;
}
        #endif
    #endif
#endif

namespace OpenKNX
{
    void TimerInterrupt::init()
    {
#if defined(ARDUINO_ARCH_RP2040)
        _alarmPool = alarm_pool_create(1, 16);
        alarm_pool_add_repeating_timer_ms(_alarmPool, -OPENKNX_INTERRUPT_TIMER_MS, timerInterruptCallback, NULL, &_repeatingTimer);
// add_repeating_timer_ms(-OPENKNX_INTERRUPT_TIMER_MS, timerInterruptCallback, NULL, &_repeatingTimer);
#elif defined(ARDUINO_ARCH_SAMD)
        ITimer.attachInterruptInterval_MS(OPENKNX_INTERRUPT_TIMER_MS, []() -> void {
            openknx.timerInterrupt.interrupt();
        });
#elif defined(ARDUINO_ARCH_ESP32)
        ITimer.attachInterrupt(OPENKNX_INTERRUPT_TIMER_MS * 1000, [](void *t) -> bool {
            openknx.timerInterrupt.interrupt();
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
        _time = millis();
        openknx.common.collectMemoryStats();

        if (_time % 2)
        {
            openknx.progLed.loop();
#ifdef INFO1_LED_PIN
            openknx.info1Led.loop();
#endif
        }
        else
        {
#ifdef INFO_LED_PIN
            openknx.infoLed.loop();
#endif
#ifdef INFO2_LED_PIN
            openknx.info2Led.loop();
#endif
        }
    }

#ifdef ARDUINO_ARCH_RP2040
    alarm_pool_t *TimerInterrupt::alarmPool()
    {
        return _alarmPool;
    }
#endif

#ifdef OPENKNX_DUALCORE
    void TimerInterrupt::init1()
    {
    #ifdef ARDUINO_ARCH_RP2040
        _alarmPool1 = alarm_pool_create(2, 16);
        alarm_pool_add_repeating_timer_ms(_alarmPool1, -OPENKNX_INTERRUPT_TIMER_MS, timerInterruptCallback1, NULL, &_repeatingTimer1);
    #endif
    }

    #ifdef __time_critical_func
    void __isr __time_critical_func(TimerInterrupt::interrupt1)()
    #else
    void TimerInterrupt::interrupt1()
    #endif
    {
        _time1 = millis();
        openknx.common.collectMemoryStats();
    }

    #ifdef ARDUINO_ARCH_RP2040
    alarm_pool_t *TimerInterrupt::alarmPool1()
    {
        return _alarmPool1;
    }
    #endif
#endif

} // namespace OpenKNX