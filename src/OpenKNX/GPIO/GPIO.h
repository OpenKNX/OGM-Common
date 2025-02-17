#pragma once

#include "Manager.h"
#include "Base.h"

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
            OPENKNX_GPIO_T_PCA9557 = 3
        };

        typedef uint16_t openknx_gpio_number_t;
    }
}