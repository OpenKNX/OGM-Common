#pragma once
#include "OpenKNX/Led/Base.h"
#include "OpenKNX/Led/RGB.h"
#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Led/GPIO_RGB.h"
#include "OpenKNX/Led/Serial.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/defines.h"
#include "knx.h"
#include <Arduino.h>
#include <string>


namespace OpenKNX
{
    namespace Led
    {
        class Functions
        {
          public:
            Functions();
            void setup();

            void loop();

            /*
             * Make a LedFunction known so a LED can be assigned to it
             * Meant to be called by Modules providing LED functions
             */
            bool RegisterLedFunction(uint32_t functionId, Led::Base* led);

            /*
             * Assign a led to a function, return true if successful
             */
            bool AssignLed2Function(Led::Base* led, uint32_t functionId);

            /*
             * Process a GroupObject for LED functions
             */
            void processInputKo(GroupObject& ko);
        };
    }
}