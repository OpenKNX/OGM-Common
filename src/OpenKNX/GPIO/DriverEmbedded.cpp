#include "DriverEmbedded.h"
#include "Arduino.h"

namespace OpenKNX
{
    namespace GPIO
    {
        struct InterruptData {
            std::function<void(openknx_gpio_number_t, bool)> callback;
            uint8_t pin;
        };

        // Interrupt handler function
        void interruptHandler(void* param) {
            // Cast the parameter back to InterruptData
            InterruptData* data = static_cast<InterruptData*>(param);

            // Call the callback with the pin and its current state
            if (data && data->callback) {
                data->callback(data->pin, digitalRead(data->pin));
            }
        }

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

        void DriverEmbedded::GPIOattachInterrupt(uint8_t pin, std::function<void(openknx_gpio_number_t, bool)> callback, PinStatus mode)
        {
            InterruptData* data = new InterruptData{callback, pin};

            // Attach the interrupt with the handler and the parameter
            ::attachInterruptParam(
                digitalPinToInterrupt(pin), // Pin interrupt
                interruptHandler,           // Interrupt handler
                mode,                       // Interrupt mode
                data                        // Parameter passed to the handler
            );
            return;
        }
    }
}