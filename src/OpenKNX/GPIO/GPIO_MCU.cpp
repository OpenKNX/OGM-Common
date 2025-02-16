#include "GPIO_MCU.h"
#include "Arduino.h"

namespace OpenKNX
{
    namespace IO
    {
        GPIO_MCU::GPIO_MCU()
        {
        }

        int GPIO_MCU::init()
        {
            return 0;
        }

        void GPIO_MCU::GPIOpinMode(uint8_t pin, int mode, bool preset, int status)
        {
            if(preset)
                digitalWriteFast(pin, status);
            pinMode(pin, mode);
        }

        void GPIO_MCU::GPIOdigitalWrite(uint8_t pin, int status)
        {
            digitalWrite(pin, status);
        }

        bool GPIO_MCU::GPIOdigitalRead(uint8_t pin)
        {
            return digitalRead(pin);
        }
    }
}