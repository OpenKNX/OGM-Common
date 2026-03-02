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
        class Base
        {
          protected:
            volatile uint32_t _lastMillis = 0;
            volatile uint8_t _maxBrightness = 255;
            volatile uint8_t _currentLedBrightness = 0;
            volatile bool _state = false;
            volatile bool _effectMode = false;
            Led::Effects::Base *_effect = nullptr;

            volatile bool _errorMode = false;
            Led::Effects::Error *_errorEffect = nullptr;

#ifdef OPENKNX_HEARTBEAT
            volatile bool _debugMode = false;
            volatile uint32_t _debugHeartbeat = 0;
            Led::Effects::Blink *_debugEffect = nullptr;
#endif
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
            /*
             * use in normal loop or loop1
             */
            virtual void loop();

            /*
             * Configure a max brightness
             */
            virtual void brightness(uint8_t brightness = 255);

            /*
             * Called by Common to Disable during SAVE Trigger
             * -> Prio 1
             */
            virtual void powerSave(bool active = true);

            /*
             * Call by fatalError to proviede error code signal
             * Code > 0: x Blink with long pause
             * Code = 0: Disable
             * -> Prio 2
             */
            virtual void errorCode(uint8_t code = 0);

#ifdef OPENKNX_HEARTBEAT
            /*
             * Special usage to detect running loop() and loop1().
             * progLed for loop()
             * infoLed for loop1()
             * Only active if OPENKNX_HEARTBEAT or OPENKNX_HEARTBEAT_PRIO is defined
             *  -> Prio 3
             */
            virtual void debugLoop();
#endif
            /****
             * Return if led is capable of RGB colors
             */
            virtual bool isColor() { return false; }

            /*
             * For progLed called by knx Stack for active Progmode
             * -> Prio 4
             */
            virtual void forceOn(bool active = true);

            /*
             * Normal "On"
             * -> Prio 5
             */
            virtual void on(bool active = true);

            /*
             * Normal "On" with pulse effect
             * -> Prio 5
             */
            virtual void pulsing(uint16_t duration = OPENKNX_LEDEFFECT_PULSE_FREQ);

            /*
             * Normal "On" with blink effect
             * -> Prio 5
             */
            virtual void blinking(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ);

            /*
             * Normal "On" with flash effect
             * -> Prio 5
             */
            virtual void flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION);

            /*
             * Normal "On" with activity effect
             * -> Prio 5
             */
            virtual void activity(uint32_t &lastActivity, bool inverted = false);

            /*
             * Normal "Off"
             * -> Prio 5
             */
            virtual void off();

            /*
             * Set the identifier for logging
             */
            virtual void setIdentifier(uint8_t identifier) { _identifier = identifier; }

            /*
             * Unload current normal effect is available
             */
            virtual void unloadEffect();

            /*
             * Call unloadEffect() and load new normal effect
             */
            virtual void loadEffect(Led::Effects::Base *effect);

            virtual bool isDimmable();

            /*
             * Get a logPrefix as string
             */
            virtual std::string logPrefix();
        };
    } // namespace Led
} // namespace OpenKNX