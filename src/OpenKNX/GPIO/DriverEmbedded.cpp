#include "DriverEmbedded.h"
#include "Arduino.h"

namespace OpenKNX
{
    namespace GPIO
    {
        struct InterruptData
        {
            std::function<void(openknx_gpio_number_t, bool)> callback;
            uint8_t pin;
        };

        // Interrupt handler function
        void interruptHandler(void* param)
        {
            // Cast the parameter back to InterruptData
            InterruptData* data = static_cast<InterruptData*>(param);

            // Call the callback with the pin and its current state
            if (data && data->callback)
            {
                data->callback(data->pin, digitalRead(data->pin));
            }
        }

        DriverEmbedded::DriverEmbedded()
        {
        }

        int DriverEmbedded::init()
        {
            _initState = GPIOInitState::OK;
            return static_cast<int>(_initState);
        }

        void DriverEmbedded::GPIOpinMode(uint8_t pin, int mode, bool preset, int status)
        {
            pinMode(pin, mode);
            if (preset)
                digitalWriteFast(pin, status);
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
#ifdef ARDUINO_ARCH_RP2040
            ::attachInterruptParam(
                digitalPinToInterrupt(pin), // Pin interrupt
                interruptHandler,           // Interrupt handler
                mode,                       // Interrupt mode
                data                        // Parameter passed to the handler
            );
            return;
#elif defined(ARDUINO_ARCH_ESP32)
            ::attachInterruptArg(
                digitalPinToInterrupt(pin), // Pin interrupt
                interruptHandler,           // Interrupt handler
                data,                       // Parameter passed to the handler
                mode                        // Interrupt mode
            );
#else
    #pragma warning "GPIOattachInterrupt not implemented for this platform"
#endif
        }
    } // namespace GPIO
} // namespace OpenKNX