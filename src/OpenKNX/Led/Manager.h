#pragma once
#include "OpenKNX/Led/Base.h"
#include "OpenKNX/Led/RGB.h"
#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Led/GPIO_RGB.h"
#include "OpenKNX/Led/Serial.h"
#include "OpenKNX/Led/Effects/Activity.h"
#include "OpenKNX/Led/Effects/Blink.h"
#include "OpenKNX/Led/Effects/Error.h"
#include "OpenKNX/Led/Effects/Flash.h"
#include "OpenKNX/Led/Effects/Pulse.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/defines.h"
#include <Arduino.h>
#include <string>

#ifndef OPENKNX_LEDS_MAX
    #define OPENKNX_LEDS_MAX 8
#endif

namespace OpenKNX
{
    namespace Led
    {
        enum LedType
        {
            LED_TYPE_PROG,
            LED_TYPE_INFO1,
            LED_TYPE_INFO2,
            LED_TYPE_INFO3,
            LED_TYPE_USER,
            LED_TYPE_MAX
        };

        class Manager
        {
          protected:
            Led::Base* _dummyLed = new GPIO(-1);
            Led::Base* _progLed = _dummyLed;
            Led::Base* _leds[OPENKNX_LEDS_MAX] = {};
            uint8_t _ledCount = 0;
            Led::Base* _specialLeds[LED_TYPE_MAX] = {};
            bool _init = false;
#ifdef OPENKNX_SERIALLED_ENABLE
            Led::SerialLedManager* _serialLedManager = nullptr;
            uint8_t _serialLedCount = 0;
            Led::SerialLedManager* getSerialLedManager(long pin);
#endif

          public:

            Manager();

            void init();

            /*
             * Must be called by TimerInterrupt every 1ms
             */
            void timer(bool distribute = true);

            /*
             * use in normal loop or loop1
             */
            void loop();

            uint8_t addLed(Led::Base*, Led::LedType);
#ifdef OPENKNX_SERIALLED_ENABLE
            uint8_t addLed(Led::Serial*, Led::LedType);
#endif

            Led::Base* getProgLed();

            Led::Base* getLed(uint8_t);

            Led::Base* getLed(LedType);


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