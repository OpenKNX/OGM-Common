#pragma once
#include "OpenKNX/LedEffects/Blink.h"
#include "OpenKNX/LedEffects/Error.h"
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
        volatile uint32_t _lastMillis = 0;
        volatile uint8_t _brightness = 255;
        volatile bool _state = false;
        volatile bool _powerSave = false;
        volatile bool _forceOn = false;
        volatile bool _errorCode = false;
        volatile uint8_t _currentLedBrightness = 0;
        volatile LedEffect _effect = LedEffect::Normal;
        LedEffects::Error _errorEffect;
        LedEffects::Pulse _pulseEffect;
        LedEffects::Blink _blinkEffect;

// set default value for DEBUG_HEARTBEAT timeout to 1000ms
#ifdef DEBUG_HEARTBEAT
        #if DEBUG_HEARTBEAT <= 1
                #undef DEBUG_HEARTBEAT
                #define DEBUG_HEARTBEAT 1000
        #endif
#endif

// set default value for DEBUG_HEARTBEAT_PRIO timeout to 1000ms
#ifdef DEBUG_HEARTBEAT_PRIO
        #if DEBUG_HEARTBEAT_PRIO <= 1
                #undef DEBUG_HEARTBEAT_PRIO
                #define DEBUG_HEARTBEAT_PRIO 1000
        #endif
#endif

#if defined(DEBUG_HEARTBEAT) || defined(DEBUG_HEARTBEAT_PRIO)
        volatile bool _debugMode = false;
        volatile uint32_t _debugHeartbeat = 0;
        LedEffects::Blink _debugEffect;

        #ifndef DEBUG_HEARTBEAT_PRIO_ON_FREQ
        #define DEBUG_HEARTBEAT_PRIO_ON_FREQ 200
        #endif
        #ifndef DEBUG_HEARTBEAT_PRIO_OFF_FREQ
        #define DEBUG_HEARTBEAT_PRIO_OFF_FREQ 1000
        #endif
        #ifndef DEBUG_HEARTBEAT_FREQ
        #define DEBUG_HEARTBEAT_FREQ 200
        #endif
#endif


        /*
         * write led state based on bool
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
        void on(bool active = true);

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

#if defined(DEBUG_HEARTBEAT) || defined(DEBUG_HEARTBEAT_PRIO)
        /*
         * Special usage to dectect running loop() and loop2().
         * progLed for loop()
         * infoLed for loop()
         * Only active if DEBUG_HEARTBEAT
         */
        void debugLoop();
#endif
    };
} // namespace OpenKNX