#pragma once
#include "OpenKNX/Led/Abstract.h"

namespace OpenKNX
{
    namespace Led
    {
        class Base: public Abstract
        {
          protected:
            volatile uint32_t _lastMillis = 0;
            volatile uint8_t _maxBrightness = 100;
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
            void powerSave(bool active = true) override;

            /*
             * Call by fatalError to proviede error code signal
             * Code > 0: x Blink with long pause
             * Code = 0: Disable
             * -> Prio 2
             */
            void errorCode(uint8_t code = 0) override;

#ifdef OPENKNX_HEARTBEAT
            /*
             * Special usage to detect running loop() and loop1().
             * progLed for loop()
             * infoLed for loop1()
             * Only active if OPENKNX_HEARTBEAT or OPENKNX_HEARTBEAT_PRIO is defined
             *  -> Prio 3
             */
            void debugLoop() override;
#endif
            /*
             * For progLed called by knx Stack for active Progmode
             * -> Prio 4
             */
            void forceOn(bool active = true) override;

            /*
             * Normal "On"
             * -> Prio 5
             */
            void on(bool active = true) override;

            /*
             * Normal "On" with pulse effect
             * -> Prio 5
             */
            void pulsing(uint16_t duration = OPENKNX_LEDEFFECT_PULSE_FREQ) override;

            /*
             * Normal "On" with blink effect
             * -> Prio 5
             */
            void blinking(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ) override;

            /*
             * Normal "On" with flash effect
             * -> Prio 5
             */
            void flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION) override;

            /*
             * Normal "On" with activity effect
             * -> Prio 5
             */
            void activity(uint32_t &lastActivity, bool inverted = false) override;

            /*
             * Normal "Off"
             * -> Prio 5
             */
            void off() override;

            /*
             * Unload current normal effect is available
             */
            void unloadEffect();

            /*
             * Call unloadEffect() and load new normal effect
             */
            void loadEffect(Led::Effects::Base *effect);


            virtual bool isDimmable() override;

            /*
             * Get a logPrefix as string
             */
            std::string logPrefix() override;
        };
    } // namespace Led
} // namespace OpenKNX