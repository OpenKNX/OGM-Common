#include "IO.h"
#include "TCA9555.h"

namespace OpenKNX
{
    namespace IO
    {

        class GPIO_TCA9555 : public iGPIOExpander
        {
          private:
            TCA9555* _tca = nullptr;
          public:
            GPIO_TCA9555(uint16_t i2cAddr, TwoWire* wire);
            virtual int init() override;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) override;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) override;
            virtual bool GPIOdigitalRead(uint8_t pin) override;
        };
    }
}