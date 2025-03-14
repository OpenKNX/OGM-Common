#include "DriverEmbedded.h"
#include "Arduino.h"

namespace OpenKNX
{
    namespace GPIO
    {
        DriverEmbedded::DriverEmbedded()
        {
        }

        int DriverEmbedded::init()
        {
            return 0;
        }

        void DriverEmbedded::GPIOpinMode(uint8_t pin, int mode, bool preset, int status)
        {
            if(preset)
                digitalWriteFast(pin, status);
            pinMode(pin, mode);
        }

        void DriverEmbedded::GPIOdigitalWrite(uint8_t pin, int status)
        {
            digitalWrite(pin, status);
        }

        bool DriverEmbedded::GPIOdigitalRead(uint8_t pin)
        {
            return digitalRead(pin);
        }

        int DriverEmbedded::GPIOattachInterrupt(uint8_t pin, void (*callback)(void), PinStatus mode)
        {
            ::attachInterrupt(digitalPinToInterrupt(pin), callback, mode);
            return 0;
        }
    }
}