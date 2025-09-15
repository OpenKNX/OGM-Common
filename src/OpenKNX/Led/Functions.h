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
#include <vector>
#include <unordered_map>


namespace OpenKNX
{
    namespace Led
    {
        enum Capability
        {
            ALL = 0xFF,
            MONOCHROME = 1,
            COLOR = 2,
        };

        class FunctionGroup
        {
            private:
              uint32_t _functionId = 0;
              std::vector<Led::Base*> _leds;
            public:
            // manage the FunctionGroup itself
              FunctionGroup(uint32_t functionId) : _functionId(functionId) {}
              void addLed(Led::Base* led);
            // control the leds assigned to this function
              void on(bool state, Capability capability = ALL);
              void on(Capability capability = ALL);
              void off(Capability capability = ALL);
              void setColor(uint8_t r, uint8_t g, uint8_t b);
              void pulsing(uint16_t duration = OPENKNX_LEDEFFECT_PULSE_FREQ, Capability capability = ALL);
              void blinking(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ, Capability capability = ALL);
              void flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION, Capability capability = ALL);
        };

        class Functions
        {
          private:
            std::unordered_map<uint32_t, FunctionGroup> _functionGroups;
          public:
            Functions();
            void setup();

            /*
             * Assign a led to a LED function
             */
            void AssignLed2Function(Led::Base* led, uint32_t functionId);

            /*
             * returns a pointer to a function group representing all leds assigned to the functionId
             */
            FunctionGroup* get(uint32_t functionId);
        };
    }
}