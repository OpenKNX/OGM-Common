#pragma once
#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Led/GPIO_RGB.h"
#include "OpenKNX/Led/RGB.h"
#include "OpenKNX/Led/Serial.h"
#include "OpenKNX/Log/Logger.h"
#include "OpenKNX/defines.h"
#include "knx.h"
#include <Arduino.h>
#include <string>
#include <unordered_map>
#include <vector>

#define OPENKNX_LEDFUNC_BASE_PROG 1
#define OPENKNX_LEDFUNC_BASE_STATE 2
#define OPENKNX_LEDFUNC_BASE_KNX 3
#define OPENKNX_LEDFUNC_BASE_TIME 4
#define OPENKNX_LEDFUNC_NET_STATE 10

namespace OpenKNX
{
    namespace Led
    {
        enum class Capability
        {
            ALL = 0xFF,
            MONOCHROME = 1,
            COLOR = 2,
        };

        /// @brief a Led::FunctionGroup represents a group of n Led::Base Leds which can be adressed by the application by the assigned FunctionID
        class FunctionGroup
        {
          private:
            bool _active = false;
            uint32_t _functionId = 0;
            std::vector<Led::Base*> _leds;
            std::string logPrefix();

          public:
            // manage the FunctionGroup itself
            FunctionGroup(uint32_t functionId) : _functionId(functionId) {}
            bool active();
            void addLed(Led::Base* led);
            // control the leds assigned to this function
            void on(bool state, Capability capability = Capability::ALL);
            void on(Capability capability = Capability::ALL);
            void off(Capability capability = Capability::ALL);
            void color(uint8_t r, uint8_t g, uint8_t b);
            void color(uint32_t rgb);
            void color(Color value);
            void pulsing(uint16_t duration = OPENKNX_LEDEFFECT_PULSE_FREQ, Capability capability = Capability::ALL);
            void blinking(uint16_t frequency = OPENKNX_LEDEFFECT_BLINK_FREQ, Capability capability = Capability::ALL);
            void flash(uint16_t duration = OPENKNX_LEDEFFECT_FLASH_DURATION, Capability capability = Capability::ALL);
            void forceOn(bool active = true);
            bool getState();
#ifdef OPENKNX_HEARTBEAT
            void debugLoop();
#endif
            void powerSave(bool active = true);
            void errorCode(uint8_t code = 0);
            void activity(uint32_t& lastActivity, bool inverted = false, Capability capability = Capability::ALL);
        };

        /// @brief manages the FunctionGroup objects present in one application
        class FunctionManager
        {
          private:
            std::unordered_map<uint32_t, FunctionGroup> _functionGroups;
            std::string logPrefix();

          public:
            FunctionManager();

            /// @brief called by Common, do not call in the application
            void init();

            /// @brief called by Common, do not call in the application
            void setup();

            /*
             * Assign a led to a LED function
             */
            void assignLed2Function(Led::Base* led, uint32_t functionId);

            /*
             * Return true if...
             * ... knx is unconfigured
             * ... ParamBASE_DefaultLedFunc is true
             */
            bool useDefaultFunction();

            /*
             * returns a pointer to a function group representing all leds assigned to the functionId, will create the group if not existing
             */
            FunctionGroup* get(uint32_t functionId);
        };

    } // namespace Led
} // namespace OpenKNX