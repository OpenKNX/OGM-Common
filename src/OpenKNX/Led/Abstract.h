#pragma once
#include "OpenKNX/Led/Effects/Blink.h"
#include "OpenKNX/Led/Effects/Error.h"
#include "OpenKNX/Led/Effects/Flash.h"
#include "OpenKNX/Led/Effects/Pulse.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/defines.h"
#include <Arduino.h>
#include <string>

namespace OpenKNX
{
    namespace Led
    {
        class Abstract
        {
          protected:
            volatile uint8_t _identifier = -1;
            volatile bool _initialized = false;
            volatile bool _powerSave = false;
            volatile bool _forceOn = false;

            /*
             * write led state based on bool
             */
            void writeLed(bool state) { writeLed((uint8_t)(state ? 255 : 0)); }
            /*
             * write led state based on bool and _brightness
             */
            virtual void writeLed(uint8_t brightness) = 0;

          public:
            virtual void init() = 0;

            /****
             * Called by Common to Disable during SAVE Trigger
             * -> Prio 1
             */
            virtual void powerSave(bool active = true) = 0;

            /****
             * Call by fatalError to proviede error code signal
             * Code > 0: x Blink with long pause
             * Code = 0: Disable
             * -> Prio 2
             */
            virtual void errorCode(uint8_t code = 0) = 0;

#ifdef OPENKNX_HEARTBEAT
            /*
             * Special usage to detect running loop() and loop1().
             * progLed for loop()
             * infoLed for loop1()
             * Only active if OPENKNX_HEARTBEAT or OPENKNX_HEARTBEAT_PRIO is defined
             *  -> Prio 3
             */
            virtual void debugLoop() {};

            #endif

            /****
             * Return if led is capable of RGB colors
             */
            virtual bool isRGB() { return false; }

            /****
             * For progLed called by knx Stack for active Progmode
             * -> Prio 4
             */
            virtual void forceOn(bool active = true) = 0;

            /****
             * Normal "On"
             * -> Prio 5
             */
            virtual void on(bool active = true) = 0;

            /****
             * Normal "On" with pulse effect
             * -> Prio 5
             */
            virtual void pulsing(uint16_t duration = OPENKNX_LEDEFFECT_PULSE_FREQ) = 0;

            /****
             * Normal "On" with blink effect
             * -> Prio 5
             */
            virtual void blinking(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ) = 0;

            /*
             * Normal "On" with flash effect
             * -> Prio 5
             */
            virtual void flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION) = 0;

            /****
             * Normal "On" with activity effect
             * -> Prio 5
             */
            virtual void activity(uint32_t &lastActivity, bool inverted = false) = 0;

            /*
             * Normal "Off"
             * -> Prio 5
             */
            virtual void off() { on(false); }

            /*
             * Set the identifier for logging
             */
            virtual void setIdentifier(uint8_t identifier) { _identifier = identifier; }

            /****
             * dimmable?
             */
            virtual bool isDimmable() { return false; }

            /*
             * Get a logPrefix as string
             */
            virtual std::string logPrefix() { return "LED-Abstract"; }
        };
    } // namespace Led
} // namespace OpenKNX