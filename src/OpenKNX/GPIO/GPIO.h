#pragma once

#include <Arduino.h>
#include <functional>

typedef uint16_t openknx_gpio_number_t;
#ifdef ARDUINO_ARCH_ESP32
    #define PinStatus uint8_t
    #define digitalWriteFast(x, y) digitalWrite(x, y)
#endif

#include "Base.h"
#include "Manager.h"

namespace OpenKNX
{
    namespace GPIO
    {
        /// @brief enum for the type of OpenKNX GPIO driver
        enum OPENKNX_GPIO_T
        {
            OPENKNX_GPIO_T_EMBEDDED = 0,
            OPENKNX_GPIO_T_TCA9555 = 1,
            OPENKNX_GPIO_T_TCA6408 = 2,
            OPENKNX_GPIO_T_PCA9557 = 3,
            OPENKNX_GPIO_T_PCA9554 = 4
        };
    } // namespace GPIO
} // namespace OpenKNX