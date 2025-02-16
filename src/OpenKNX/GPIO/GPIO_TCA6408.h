#include "IO.h"
#include "TCA6408.h"

namespace OpenKNX
{
    namespace IO
    {

        class GPIO_TCA6408 : public iGPIOExpander
        {
          private:
            TCA6408* _tca = nullptr;
          public:
            GPIO_TCA6408(uint16_t i2cAddr, TwoWire* wire);
            virtual int init() override;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) override;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) override;
            virtual bool GPIOdigitalRead(uint8_t pin) override;
        };
    }
}