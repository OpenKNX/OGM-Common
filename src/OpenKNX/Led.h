#pragma once
#include "OpenKNX/LedEffects/Blink.h"
#include "OpenKNX/LedEffects/Error.h"
#include "OpenKNX/LedEffects/Pulse.h"
#include "knx.h"
#include <string>

//
// Debug Heartbeat
//

// Priority active?
#ifdef OPENKNX_HEARTBEAT_PRIO

// Remove double define
#undef OPENKNX_HEARTBEAT
#ifndef OPENKNX_HEARTBEAT
#define OPENKNX_HEARTBEAT OPENKNX_HEARTBEAT_PRIO
#endif
#endif

// Heartbeat active? (with priority)
#ifdef OPENKNX_HEARTBEAT

// set default value for OPENKNX_HEARTBEAT timeout to 1000ms
#if OPENKNX_HEARTBEAT <= 1
#undef OPENKNX_HEARTBEAT
#define OPENKNX_HEARTBEAT 1000
#endif

// set default frequencies (normal)
#ifndef OPENKNX_HEARTBEAT_FREQ
#define OPENKNX_HEARTBEAT_FREQ 200
#endif

// set default frequencies (prio with active forceOn)
#ifndef OPENKNX_HEARTBEAT_PRIO_ON_FREQ
#define OPENKNX_HEARTBEAT_PRIO_ON_FREQ 200
#endif

// set default frequencies (prio with inactive forceOn)
#ifndef OPENKNX_HEARTBEAT_PRIO_OFF_FREQ
#define OPENKNX_HEARTBEAT_PRIO_OFF_FREQ 1000
#endif
#endif

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
        volatile long _pin = -1;
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

#ifdef OPENKNX_HEARTBEAT
        volatile bool _debugMode = false;
        volatile uint32_t _debugHeartbeat = 0;
        LedEffects::Blink _debugEffect;
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
         * Normal "Off"
         * -> Prio 5
         */
        void off();

        /*
         * Get a logPrefix as string
         */
        std::string logPrefix();
    };
} // namespace OpenKNX