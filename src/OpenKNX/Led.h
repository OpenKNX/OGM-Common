#pragma once
#include "OpenKNX/LedEffects/Blink.h"
#include "OpenKNX/LedEffects/Pulse.h"
#include "knx.h"

namespace OpenKNX
{
    enum class LedEffect
    {
        Normal,
        Blink,
        Pulse,
        Error
    };

    class Led
    {
      private:
        volatile long _pin = 1;
        volatile long _activeOn = HIGH;
        volatile uint32_t _lastMicros = 0;
        volatile uint8_t _brightness = 255;
        volatile bool _state = false;
        volatile bool _powerSave = false;
        volatile bool _forceOn = false;
        volatile uint8_t _errorCode = 0;
        volatile uint8_t _currentLedBrightness = 0;
        volatile LedEffect _effect = LedEffect::Normal;
        LedEffects::Pulse _pulseEffect;
        LedEffects::Blink _blinkEffect;

#ifdef OPENKNX_DEBUG_LOOP
        volatile bool _debugMode = false;
        volatile bool _debugLast = false;
        volatile uint32_t _debugMicros = 0;
        LedEffects::Blink _debugEffect;
#endif
        /*
         * write led state based on boo
         */
        void writeLed(bool state);
        /*
         * write led state based on bool and _brightness
         */
        void writeLed(uint8_t brightness);

      public:
        void init(long pin = -1, long activeOn = HIGH);

        /*
         * use in normal loop or loop2
         */
        void loop();

        /*
         * used for progLed and infoLed in interruptTimer
         */
        void loop(uint32_t micros);

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
         * For progLed called by knx Stack for active Progmode
         * -> Prio 2
         */
        void forceOn(bool active = true);
        /*
         * Call by fatalError to proviede error code signal
         * Code > 0: x Blink with long pause
         * Code = 0: Disable
         * -> Prio 3
         */
        void errorCode(uint8_t code = 0);

        /*
         * Normal "On"
         * -> Prio 4
         */
        void on();
        
        /*
         * Normal "On" with pulse effect
         * -> Prio 4
         */
        void pulsing(uint16_t duration = OPENKNX_LEDEFFECT_PULSE_FREQ);

        /*
         * Normal "On" with blink effect
         * -> Prio 4
         */
        void blinking(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ);

        /*
         * Normal "Off"
         * -> Prio 4
         */
        void off();

#ifdef OPENKNX_DEBUG_LOOP
        /*
         * Special usage to dectect running loop() and loop2().
         * progLed for loop()
         * infoLed for loop()
         * Only active if OPENKNX_DEBUG_LOOP
         */
        void debugLoop();
#endif
    };
} // namespace OpenKNX