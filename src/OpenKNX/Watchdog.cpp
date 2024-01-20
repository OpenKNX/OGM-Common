#include "OpenKNX/Watchdog.h"
#include "OpenKNX/Facade.h"

#ifdef OPENKNX_WATCHDOG
    #ifdef OPENKNX_DEBUGGER
        #pragma message "Disable watchdog because OPENKNX_DEBUGGER is defined"
        #undef OPENKNX_WATCHDOG
    #endif
#endif

#ifdef ARDUINO_ARCH_RP2040
/*
 * 166 != Uninitalized (safe reboot or powerloss, maybe a new firmware)
 * 166 = Normal running (after reboot = restart by watchdog or maybe flash firmware)
 */
uint8_t __uninitialized_ram(__openKnxRunningState);

/*
 * Counting the restarts of the WD over the restart
 */
uint8_t __uninitialized_ram(__openKnxWatchdogResets);
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
        return _resets;
    }

    bool Watchdog::lastReset()
    {
        return _lastReset;
    }

    void Watchdog::safeRestart()
    {
#ifdef OPENKNX_WATCHDOG
    #ifdef ARDUINO_ARCH_RP2040
        __openKnxRunningState = 0;
    #endif
#endif
    }

    void Watchdog::loop()
    {
        if (!active()) return;

#ifdef OPENKNX_WATCHDOG
        ::Watchdog.reset();
#endif
    }

    void Watchdog::activate()
    {
#ifdef OPENKNX_WATCHDOG
        logInfo("Watchdog", "Start with a watchtime of %ims", OPENKNX_WATCHDOG_MAX_PERIOD);
        _active = true;
    #ifdef ARDUINO_ARCH_RP2040
        __openKnxRunningState = 166;
    #endif
    #ifdef ARDUINO_ARCH_SAMD
        ::Watchdog.enable(OPENKNX_WATCHDOG_MAX_PERIOD, false);
    #else
        ::Watchdog.enable(OPENKNX_WATCHDOG_MAX_PERIOD);
    #endif
#endif
    }

    void Watchdog::deactivate()
    {
        _active = false;
#ifdef OPENKNX_WATCHDOG
    #ifdef ARDUINO_ARCH_RP2040
        __openKnxRunningState = 0;
        ::Watchdog.enable(2147483647);
    #else
        ::Watchdog.disable();
    #endif
#endif
    }

    Watchdog::Watchdog()
    {
#ifdef OPENKNX_WATCHDOG

    #ifdef ARDUINO_ARCH_RP2040
        if (__openKnxRunningState == 166)
        {
            // system was rebooted by watchdog or flash firmware
            _lastReset = true;
            if (__openKnxWatchdogResets < 255) __openKnxWatchdogResets++;
        }
        else
        {
            __openKnxWatchdogResets = 0;
        }

        _resets = __openKnxWatchdogResets;
    #endif

    #ifdef ARDUINO_ARCH_SAMD
        if (::Watchdog.resetCause() & PM_RCAUSE_WDT)
        {
            _lastReset = true;
            _resets = 1;
        }
    #endif
#endif
    }
} // namespace OpenKNX