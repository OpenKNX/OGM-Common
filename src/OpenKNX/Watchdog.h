#pragma once
#include <Arduino.h>

#include "OpenKNX/defines.h"

#ifndef OPENKNX_WATCHDOG_AUTOERASE_RESETS
    #define OPENKNX_WATCHDOG_AUTOERASE_RESETS 5
#endif
#ifndef OPENKNX_WATCHDOG_AUTOERASE_TIMEOUT
    #define OPENKNX_WATCHDOG_AUTOERASE_TIMEOUT 30
#endif

namespace OpenKNX
{
    class Watchdog
    {

      public:
        /*
         * Activate by common
         */
        void activate();

        /*
         * Deactivate or alternative set a very long time
         */
        void deactivate();

        /*
         * Return is watchdog enabled
         */
        bool active();

        /*
         * Returns the number of consecutive resets by the watchdog
         */
        uint8_t resets();

        /*
         * Returns whether the last restart was performed by the watchdog
         * Hint: On RP2040 it is possible that reset by flashing firmware also looks like wd reset
         */
        bool lastReset();

        /*
         * Must be called regularly so that the watchdog does not strike
         */
        void loop();

        /*
         * Returns the maximum time until the watchdog resets the device
         */
        uint32_t maxPeriod();

        /*
         * Called before knx init
         */
        void fastCheck();

        /*
         * Mark that setup completed with a valid config loaded. Used so the auto-erase
         * only triggers for an unloadable config (crash before setup completes), never
         * for a runtime crash (e.g. IP flood / heap exhaustion) after a healthy boot.
         */
        void markSetupCompleted();

        String logPrefix()
        {
            return String("Watchdog");
        };

        Watchdog();

      private:
        bool _active = false;
        bool _lastReset = false;
        bool _first = true;
    };
} // namespace OpenKNX
