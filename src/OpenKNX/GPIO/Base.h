#pragma once
#include <Arduino.h>

namespace OpenKNX
{
    namespace GPIO
    {
        class Base
        {
        public:
            virtual int init() = 0;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) = 0;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) = 0;
            virtual bool GPIOdigitalRead(uint8_t pin) = 0;
            virtual int GPIOattachInterrupt(uint8_t pin, std::function<void(openknx_gpio_number_t, int)> callback, PinStatus mode)
            {
                return -1;
            }
        };
    }
}