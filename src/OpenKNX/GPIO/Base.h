#pragma once
#include <Arduino.h>

namespace OpenKNX
{
    namespace GPIO
    {
        enum class GPIOInitState : int
        {
            OK = 0,             // Initialisation successful
            Failed = -1,        // Initialisation failed
            Uninitialized = -2, // GPIO not initialized
            TypeUnknown = -1000 // Expander type unknown
        };

        class Base
        {
          public:
            virtual int init() = 0;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) = 0;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) = 0;
            virtual bool GPIOdigitalRead(uint8_t pin) = 0;
            virtual void GPIOattachInterrupt(uint8_t pin, std::function<void(openknx_gpio_number_t, bool)> callback, PinStatus mode) {}

            virtual inline const bool isInitialized() { return false; }
            virtual inline void setInitState(const GPIOInitState state) {}
            virtual inline GPIOInitState getInitState() { return GPIOInitState::TypeUnknown; }
        };

    } // namespace GPIO
} // namespace OpenKNX