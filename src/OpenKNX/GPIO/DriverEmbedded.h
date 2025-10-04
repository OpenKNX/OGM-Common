#include "GPIO.h"

namespace OpenKNX
{
    namespace GPIO
    {
        /// @brief OpenKNX GPIO driver for GPIOs embedded into the MCU (RP2040, ESP32, ...). Wraps the digitalRead etc... arduino functions.
        class DriverEmbedded : public Base
        {
          public:
            DriverEmbedded();
            virtual int init() override;

            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) override;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) override;
            virtual bool GPIOdigitalRead(uint8_t pin) override;
            virtual void GPIOattachInterrupt(uint8_t pin, std::function<void(openknx_gpio_number_t, bool)> callback, PinStatus mode) override;

            virtual inline const bool isInitialized() { return _initState == GPIOInitState::OK; }
            virtual inline void setInitState(const GPIOInitState state) { _initState = state; }
            virtual inline GPIOInitState getInitState() { return _initState; }

          private:
            GPIOInitState _initState = GPIOInitState::Uninitialized;
        };
    } // namespace GPIO
} // namespace OpenKNX