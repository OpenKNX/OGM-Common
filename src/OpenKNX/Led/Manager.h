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
            Led::Base* _progLed;

          public:

            void init();

            void timer();

            /*
             * use in normal loop or loop1
             */
            void loop();

            Led::Base* getProgLed();


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