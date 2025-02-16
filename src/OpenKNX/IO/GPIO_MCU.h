#include "IO.h"

namespace OpenKNX
{
    namespace IO
    {
        class GPIO_MCU : public iGPIOExpander
        {
          public:
            GPIO_MCU();
            virtual int init() override;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) override;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) override;
            virtual bool GPIOdigitalRead(uint8_t pin) override;
        };
    }
}