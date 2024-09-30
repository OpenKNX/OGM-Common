#pragma once
#include "OpenKNX/Led/Effects/Activity.h"
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
            volatile long _pin = -1;
            volatile long _activeOn = HIGH;
            volatile uint32_t _lastMillis = 0;
            volatile uint8_t _maxBrightness = 100;
            volatile bool _state = false;
            volatile bool _powerSave = false;
            volatile bool _forceOn = false;
            volatile uint8_t _currentLedBrightness = 0;

            volatile bool _effectMode = false;
            Led::Effects::Base *_effect = nullptr;

            volatile bool _errorMode = false;
            Led::Effects::Error *_errorEffect = nullptr;

#ifdef OPENKNX_HEARTBEAT
            volatile bool _debugMode = false;
            volatile uint32_t _debugHeartbeat = 0;
            Led::Effects::Blink *_debugEffect = nullptr;
#endif

            /*
             * write led state based on bool
             */
            void writeLed(bool state);
            /*
             * write led state based on bool and _brightness
             */
            virtual void writeLed(uint8_t brightness) = 0;

          public:
            /*
             * use in normal loop or loop1
             */
            void loop();

            /*
             * Configure a max brightness
             */
            void brightness(uint8_t brightness = 255);

            /*
             * Called by Common to Disable during SAVE Trigger
             * -> Prio 1
             */
            void powerSave(bool active = true);

            /*
             * Call by fatalError to proviede error code signal
             * Code > 0: x Blink with long pause
             * Code = 0: Disable
             * -> Prio 2
             */
            void errorCode(uint8_t code = 0);

#ifdef OPENKNX_HEARTBEAT
            /*
             * Special usage to detect running loop() and loop1().
             * progLed for loop()
             * infoLed for loop1()
             * Only active if OPENKNX_HEARTBEAT or OPENKNX_HEARTBEAT_PRIO is defined
             *  -> Prio 3
             */
            void debugLoop();
#endif

            /*
             * For progLed called by knx Stack for active Progmode
             * -> Prio 4
             */
            void forceOn(bool active = true);

            /*
             * Normal "On"
             * -> Prio 5
             */
            void on(bool active = true);

            /*
             * Normal "On" with pulse effect
             * -> Prio 5
             */
            void pulsing(uint16_t duration = OPENKNX_LEDEFFECT_PULSE_FREQ);

            /*
             * Normal "On" with blink effect
             * -> Prio 5
             */
            void blinking(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ);

            /*
             * Normal "On" with flash effect
             * -> Prio 5
             */
            void flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION);

            /*
             * Normal "On" with activity effect
             * -> Prio 5
             */
            void activity(uint32_t &lastActivity, bool inverted = false);

            /*
             * Normal "Off"
             * -> Prio 5
             */
            void off();

            /*
             * Unload current normal effect is available
             */
            void unloadEffect();

            /*
             * Call unloadEffect() and load new normal effect
             */
            void loadEffect(Led::Effects::Base *effect);

            /*
             * Get a logPrefix as string
             */
            std::string logPrefix();
        };
    } // namespace Led
} // namespace OpenKNX