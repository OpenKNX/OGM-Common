#include "OpenKNX/Watchdog.h"
#include "OpenKNX/Facade.h"

#ifdef OPENKNX_WATCHDOG
    #ifdef ARDUINO_ARCH_RP2040
        #include <hardware/watchdog.h>
        #include <pico/time.h>
    #endif
    #ifdef ARDUINO_ARCH_ESP32
        #include "esp_task_wdt.h"
    #endif
    #ifdef ARDUINO_ARCH_SAMD
        #include <sam.h>
    #endif
#endif

#ifdef OPENKNX_WATCHDOG
    #ifdef OPENKNX_DEBUGGER
        #pragma message "Disable watchdog because OPENKNX_DEBUGGER is defined"
        #undef OPENKNX_WATCHDOG
    #endif
#endif

/*
 * Counting the restarts of the WD over the restart
 */
#if defined(ARDUINO_ARCH_RP2040)
static uint8_t __uninitialized_ram(__openKnxWatchdogResets);
static uint8_t __uninitialized_ram(__openKnxWatchdogFasts);
// "Previous boot reached a healthy configured runtime" marker: lets the auto-erase
// wipe the config only for an unloadable config (crash before setup completes),
// never for a runtime crash (IP flood, heap exhaustion) after a healthy boot.
static uint8_t __uninitialized_ram(__openKnxSetupCompleted);
#elif defined(ARDUINO_ARCH_ESP32)
static RTC_NOINIT_ATTR uint8_t __openKnxWatchdogResets;
static RTC_NOINIT_ATTR uint8_t __openKnxWatchdogFasts;
static RTC_NOINIT_ATTR uint8_t __openKnxSetupCompleted;
#else
// Not supported
static uint8_t __openKnxWatchdogResets = 0;
static uint8_t __openKnxWatchdogFasts = 0;
static uint8_t __openKnxSetupCompleted = 0;
#endif

namespace OpenKNX
{
    uint32_t Watchdog::maxPeriod()
    {
        return OPENKNX_WATCHDOG_MAX_PERIOD;
    }

    bool Watchdog::active()
    {
        return _active;
    }

    uint8_t Watchdog::resets()
    {
        return __openKnxWatchdogResets;
    }

    bool Watchdog::lastReset()
    {
        return _lastReset;
    }

    void Watchdog::fastCheck()
    {
        (void)__openKnxWatchdogFasts; // Suppress unused variable warning
#if defined(OPENKNX_WATCHDOG) && !defined(ARDUINO_ARCH_SAMD)
        logTraceP("fastCheck: %i/%i/%i", _lastReset, __openKnxWatchdogResets, __openKnxWatchdogFasts);
    #ifdef OPENKNX_WATCHDOG_AUTOERASE_RESETS
        if (__openKnxWatchdogFasts == OPENKNX_WATCHDOG_AUTOERASE_RESETS)
        {
            logErrorP("The knx flash is erased due to too many restarts (%i)", __openKnxWatchdogFasts);
            openknx.knxFlash.erase();
        }
    #endif
#endif
    }

    // Records that this boot reached a healthy, configured runtime. Consulted by the
    // ctor on the NEXT boot so a runtime crash (after setup) never triggers auto-erase.
    void Watchdog::markSetupCompleted()
    {
#ifdef OPENKNX_WATCHDOG
        __openKnxSetupCompleted = 1;
#endif
    }

    void Watchdog::loop()
    {
#ifdef OPENKNX_WATCHDOG

        if (_first)
        {
            logTraceP("firstLoop: %i/%i/%i", _lastReset, __openKnxWatchdogResets, __openKnxWatchdogFasts);
            _first = false;
        }
    #ifdef OPENKNX_WATCHDOG_AUTOERASE_TIMEOUT
        if (__openKnxWatchdogFasts > 0 && millis() > (OPENKNX_WATCHDOG_AUTOERASE_TIMEOUT * 1000))
        {
            logTraceP("Reset fast restarts counter (%i)", __openKnxWatchdogFasts);
            __openKnxWatchdogFasts = 0;
        }
    #endif

        if (!active()) return;
    #if defined(ARDUINO_ARCH_RP2040)
        watchdog_update();
    #elif defined(ARDUINO_ARCH_ESP32)
        esp_task_wdt_reset();
    #elif defined(ARDUINO_ARCH_SAMD)
        // ::Watchdog.reset();
        while (WDT->STATUS.bit.SYNCBUSY)
            ;
        WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
    #endif
#endif
    }

    void Watchdog::activate()
    {
        if (_active) return;
#ifdef OPENKNX_WATCHDOG
        logInfoP("Start with a watchtime of %is", OPENKNX_WATCHDOG_MAX_PERIOD);
        _active = true;
    #if defined(ARDUINO_ARCH_RP2040)
        watchdog_enable(OPENKNX_WATCHDOG_MAX_PERIOD * 1000, true);
    #elif defined(ARDUINO_ARCH_SAMD)
        GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(4);
        GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_GENEN |
                            GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_DIVSEL;
        while (GCLK->STATUS.bit.SYNCBUSY)
            ;
        GCLK->CLKCTRL.reg =
            GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2;

        NVIC_DisableIRQ(WDT_IRQn);
        NVIC_ClearPendingIRQ(WDT_IRQn);
        NVIC_SetPriority(WDT_IRQn, 0);
        NVIC_EnableIRQ(WDT_IRQn);

        WDT->CTRL.reg = 0;
        while (WDT->STATUS.bit.SYNCBUSY)
            ;

        int cycles;
        uint8_t bits;
        uint16_t maxPeriodMS = OPENKNX_WATCHDOG_MAX_PERIOD * 1000;

        if ((maxPeriodMS >= 16000) || !maxPeriodMS)
        {
            cycles = 16384;
            bits = 0xB;
        }
        else
        {
            cycles = (maxPeriodMS * 1024L + 500) / 1000; // ms -> WDT cycles
            if (cycles >= 8192)
            {
                cycles = 8192;
                bits = 0xA;
            }
            else if (cycles >= 4096)
            {
                cycles = 4096;
                bits = 0x9;
            }
            else if (cycles >= 2048)
            {
                cycles = 2048;
                bits = 0x8;
            }
            else if (cycles >= 1024)
            {
                cycles = 1024;
                bits = 0x7;
            }
            else if (cycles >= 512)
            {
                cycles = 512;
                bits = 0x6;
            }
            else if (cycles >= 256)
            {
                cycles = 256;
                bits = 0x5;
            }
            else if (cycles >= 128)
            {
                cycles = 128;
                bits = 0x4;
            }
            else if (cycles >= 64)
            {
                cycles = 64;
                bits = 0x3;
            }
            else if (cycles >= 32)
            {
                cycles = 32;
                bits = 0x2;
            }
            else if (cycles >= 16)
            {
                cycles = 16;
                bits = 0x1;
            }
            else
            {
                cycles = 8;
                bits = 0x0;
            }
        }

        WDT->INTENCLR.bit.EW = 1;
        WDT->CONFIG.bit.PER = bits;
        WDT->CTRL.bit.WEN = 0;
        while (WDT->STATUS.bit.SYNCBUSY)
            ;

        WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;

        WDT->CTRL.bit.ENABLE = 1;
        while (WDT->STATUS.bit.SYNCBUSY)
            ;
    #elif defined(ARDUINO_ARCH_ESP32)
        #if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        esp_task_wdt_config_t wdt_config = {
            .timeout_ms = OPENKNX_WATCHDOG_MAX_PERIOD * 1000,
            .idle_core_mask = 1,
            .trigger_panic = true,
        };

        // Initialisiere den Watchdog mit der Konfiguration
        esp_task_wdt_deinit();
        esp_task_wdt_init(&wdt_config);
        #else
        esp_task_wdt_init(OPENKNX_WATCHDOG_MAX_PERIOD, true);
        #endif
        esp_task_wdt_add(NULL);
    #endif
#endif
    }

    void Watchdog::deactivate()
    {
        _active = false;
#ifdef OPENKNX_WATCHDOG
    #ifdef ARDUINO_ARCH_RP2040
        watchdog_disable();
    #elif defined(ARDUINO_ARCH_SAMD)
        WDT->CTRL.bit.ENABLE = 0;
        while (WDT->STATUS.bit.SYNCBUSY)
            ;
    #elif defined(ARDUINO_ARCH_ESP32)
        esp_task_wdt_delete(NULL);
    #endif
#endif
    }

    Watchdog::Watchdog()
    {
#ifdef OPENKNX_WATCHDOG

    #ifdef ARDUINO_ARCH_RP2040
        if (watchdog_enable_caused_reboot())
        {
            // system was rebooted by watchdog or flash firmware
            _lastReset = true;
        }
    #endif

    #ifdef ARDUINO_ARCH_ESP32
        esp_reset_reason_t reason = esp_reset_reason();
        if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_PANIC || reason == ESP_RST_WDT || reason == ESP_RST_BROWNOUT)
        {
            _lastReset = true;
        }
    #endif

    #ifdef ARDUINO_ARCH_SAMD
        if (PM->RCAUSE.reg & PM_RCAUSE_WDT)
        {
            _lastReset = true;
        }
    #endif

        if (_lastReset)
        {
            if (__openKnxWatchdogResets < 255) __openKnxWatchdogResets++;
    #ifdef OPENKNX_WATCHDOG_AUTOERASE_RESETS
            // Only count toward auto-erase if the PREVIOUS boot did NOT reach a
            // healthy configured runtime. A WDT/panic reset after setup completed is
            // a runtime crash (IP flood, heap exhaustion, ...) - NOT an unloadable
            // config - so it must never drive the fast counter that wipes the KNX
            // config. __openKnxSetupCompleted is carried over from the last boot
            // (set at the end of setup, re-armed to 0 below for this boot).
            if (!__openKnxSetupCompleted)
            {
                if (__openKnxWatchdogFasts < 255) __openKnxWatchdogFasts++;
            }
    #endif
        }
        else
        {
            __openKnxWatchdogResets = 0;
            __openKnxWatchdogFasts = 0;
        }

        // Re-arm for THIS boot: cleared now, set back to 1 only if setup() reaches a
        // healthy configured runtime (Watchdog::markSetupCompleted()). So the next
        // boot's classification above reflects whether THIS boot booted cleanly.
        __openKnxSetupCompleted = 0;

#endif
    }
} // namespace OpenKNX