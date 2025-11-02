#pragma once
#include "OpenKNX/Led/Base.h"
#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Led/GPIO_RGB.h"
#include "OpenKNX/Led/RGB.h"
#include "OpenKNX/Led/Serial.h"
#include "OpenKNX/defines.h"
#include "knx.h"
#include <Arduino.h>
#include <string>

namespace OpenKNX
{
    namespace Led
    {
        /// @brief provides some basic LedFunctions of Common
        class Functions
        {
          private:
          public:
            Functions();
            void init();
            void setup();
            void loop();
        };
    } // namespace Led
} // namespace OpenKNX