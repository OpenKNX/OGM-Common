#pragma once
#include "OpenKNX/Led/Base.h"
#include "OpenKNX/Led/Effects/Activity.h"
#include "OpenKNX/Led/Effects/Blink.h"
#include "OpenKNX/Led/Effects/Error.h"
#include "OpenKNX/Led/Effects/Flash.h"
#include "OpenKNX/Led/Effects/Pulse.h"
#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Led/GPIO_RGB.h"
#include "OpenKNX/Led/RGB.h"
#include "OpenKNX/Led/Serial.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/defines.h"
#include <Arduino.h>
#include <string>
#include <unordered_map>

#ifndef OPENKNX_LEDS_MAX
    #define OPENKNX_LEDS_MAX 8
#endif

namespace OpenKNX
{
    namespace Led
    {
        /*
         * Predefined LED identifiers
         * For User LED use LED_TYPE_USER + x (x = 0..n)
         */
        enum LedType
        {
            LED_TYPE_PROG = 0,
            LED_TYPE_INFO1 = 1,
            LED_TYPE_INFO2 = 2,
            LED_TYPE_INFO3 = 3,
            LED_TYPE_USER = 10
        };

        class Manager
        {
          protected:
            Led::Base* _dummyLed = new GPIO(-1);
            std::unordered_map<uint8_t, Led::Base*> _leds;
            bool _init = false;
            uint32_t _timerMillis = 0;
#ifdef OPENKNX_SERIALLED_ENABLE
            Led::SerialLedManager* _serialLedManager = nullptr;
            uint8_t _serialLedCount = 0;
            Led::SerialLedManager* getSerialLedManager(long pin);
#endif

          public:
            Manager();

            void init();

            /*
             * Should be called by a TimerInterrupt or Task with at least 100Hz
             */
            void timer(bool doNotCheckMillis = false);

            /*
             * use in normal loop or loop1
             */
            void loop();

            void addLed(Led::Base*, uint8_t);
#ifdef OPENKNX_SERIALLED_ENABLE
            void addLed(Led::Serial*, uint8_t);
#endif
            /*
             *gets the ProgLed, dummyLed if not available
             */
            Led::Base* getProgLed();

            /*
             *gets the Led for identifier, dummyLed if not available
             */
            Led::Base* getLed(uint8_t identifier);

            /*
             * Called by Common to Disable during SAVE Trigger
             * -> Prio 1
             */
            void powerSave(bool active = true);

            /*
             * Get a logPrefix as string
             */
            std::string logPrefix();
        };
    } // namespace Led
} // namespace OpenKNX