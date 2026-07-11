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
        #define SELECTED_TIMER FREERTOS
    //     #if OPENKNX_TIMER_INTERRUPT >= 0 && OPENKNX_TIMER_INTERRUPT <= 3
    //         #define SELECTED_TIMER OPENKNX_TIMER_INTERRUPT
    //     #endif
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

    #ifndef ARDUINO_ARCH_ESP32
        // include after defines!
        #include "TimerInterrupt_Generic.h"
    #endif

    // Select Timer Interrupt
    #if defined(ARDUINO_ARCH_SAMD)
SAMDTimer ITimer(SELECTED_TIMER);
    //     #elif defined(ARDUINO_ARCH_ESP32)
    // ESP32Timer ITimer(SELECTED_TIMER);
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

#ifdef ARDUINO_ARCH_ESP32
// Anti-flicker: the blocking LED-I2C flush runs in its own task so it never delays the PWM tick.
static TaskHandle_t s_ledFlushTask = nullptr;
static void openKnxLedFlushTask(void *)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for the PWM tick to signal fresh pending states
        openknx.leds.flushI2C();
    }
}
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
#elif SELECTED_TIMER == FREERTOS
    #ifdef ARDUINO_ARCH_ESP32
        // esp_timer periodic timer (precise fixed period, mirror of the RP2040 alarm pool);
        // the callback runs in the esp_timer task and must stay short/non-blocking.
        // Create the flush task first so its handle is valid before the tick can notify it.
        // Priority below the esp_timer service task (so the tick always preempts) but above the
        // Arduino loop + lwIP + display render, so the flush wins the Wire1 mutex between page chunks.
        xTaskCreatePinnedToCore(openKnxLedFlushTask, "OpenKnxLedFlush", 4096, nullptr,
                                configMAX_PRIORITIES - 4, &s_ledFlushTask, tskNO_AFFINITY);

        const esp_timer_create_args_t timerArgs = {
            .callback = [](void *arg) {
                openknx.timerInterrupt.interrupt(); // PWM tick: compute pending states, no I2C
                if (s_ledFlushTask)
                    xTaskNotifyGive(s_ledFlushTask);
            },
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "OpenKnxTimer0",
            .skip_unhandled_events = true};
        esp_timer_create(&timerArgs, &_espTimer);
        esp_timer_start_periodic(_espTimer, (uint64_t)OPENKNX_INTERRUPT_TIMER_MS * 1000ULL);
    #endif
#endif
    }

    void __isr __time_critical_func(TimerInterrupt::interrupt)()
    {
        _time = millis();

        processStats();
        processLeds();
        processButtons();
    }

    void TimerInterrupt::processStats()
    {
#ifdef ARDUINO_ARCH_RP2040
        openknx.common.collectStackStats();
#endif
        openknx.common.collectHeapStats();
    }

    void TimerInterrupt::processButtons()
    {
        openknx.progButton.loop();
#ifdef FUNC1_BUTTON_PIN
        openknx.func1Button.loop();
#endif
#ifdef FUNC2_BUTTON_PIN
        openknx.func2Button.loop();
#endif
#ifdef FUNC3_BUTTON_PIN
        openknx.func3Button.loop();
#endif
    }

    void TimerInterrupt::processLeds()
    {
#ifdef OPENKNX_SERIALLED_ENABLE
    #ifdef ARDUINO_ARCH_ESP32
        return;
    #endif
#endif
        openknx.leds.timer();
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
    #elif SELECTED_TIMER == FREERTOS
        #ifdef ARDUINO_ARCH_ESP32
        // esp_timer periodic timer for the second core (parity with init())
        const esp_timer_create_args_t timerArgs1 = {
            .callback = [](void *arg) {
                openknx.timerInterrupt.interrupt1();
            },
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "OpenKnxTimer1",
            .skip_unhandled_events = true};
        esp_timer_create(&timerArgs1, &_espTimer1);
        esp_timer_start_periodic(_espTimer1, (uint64_t)OPENKNX_INTERRUPT_TIMER_MS * 1000ULL);
        #endif
    #endif
    }

    void __isr __time_critical_func(TimerInterrupt::interrupt1)()
    {
        _time1 = millis();
        processStats();
        processLeds();
    }

    #ifdef ARDUINO_ARCH_RP2040
    alarm_pool_t *TimerInterrupt::alarmPool1()
    {
        return _alarmPool1;
    }
    #endif
#endif

} // namespace OpenKNX