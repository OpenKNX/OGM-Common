#include "GPIO.h"
#include "Libs/TCA9555.h"

namespace OpenKNX
{
    namespace GPIO
    {
        class DriverTCA9555 : public Base
        {
          private:
            TCA9555* _tca = nullptr;
          public:
            DriverTCA9555(uint16_t i2cAddr, TwoWire* wire);
            virtual int init() override;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) override;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) override;
            virtual bool GPIOdigitalRead(uint8_t pin) override;
            //virtual void GPIOattachInterrupt(uint8_t pin, std::function<void(openknx_gpio_number_t, bool)> callback, PinStatus mode) override;
        };
    }
}