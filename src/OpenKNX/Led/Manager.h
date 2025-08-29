#pragma once
#include "OpenKNX/Led/Base.h"
#include "OpenKNX/Led/GPIO.h"
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
            LED_TYPE_PROG = 0,
            LED_TYPE_INFO1 = 1,
            LED_TYPE_USER = 10
        };

        class Manager
        {
          protected:
            Led::Base* _dummyLed = new GPIO(-1);
            Led::Base* _progLed = _dummyLed;
            Led::Base* _leds[OPENKNX_LEDS_MAX];
            uint8_t _ledCount = 0;
            bool _init = false;

          public:

            void init();

            void timer();

            /*
             * use in normal loop or loop1
             */
            void loop();

            bool addLed(Led::Base*, Led::LedType);

            Led::Base* getProgLed();

            Led::Base* getLed(uint8_t);


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