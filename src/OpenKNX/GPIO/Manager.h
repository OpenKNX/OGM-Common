#pragma once
#include "GPIO.h"
#include "hardware.h"
#include <Arduino.h>
#include <string>

#ifndef OPENKNX_GPIO_CLOCK
    #define OPENKNX_GPIO_CLOCK 50000
#endif

namespace OpenKNX
{
    namespace GPIO
    {
        /// @brief OpenKNX GPIO Manager. Acts as abstraction layer for GPIOs embedded in the MCU and GPIOs provided by port expanders.
        class Manager
        {
          public:
            Manager();
            ~Manager();

            void init();
            void showInitResults();
            void loop();

            std::string logPrefix();

            void pinMode(openknx_gpio_number_t pin, int mode, bool preset = false, int status = 0);
            void digitalWrite(openknx_gpio_number_t pin, int status);
            bool digitalRead(openknx_gpio_number_t pin);
            // int attachInterrupt(openknx_gpio_number_t pin, void (*callback)(void), PinStatus mode);
            void attachInterrupt(openknx_gpio_number_t pin, std::function<void(openknx_gpio_number_t, bool)> callback, PinStatus mode);
            bool isInitialized(uint8_t expander);
        };
    } // namespace GPIO
} // namespace OpenKNX