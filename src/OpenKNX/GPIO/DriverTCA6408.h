#include "GPIO.h"
#include "Libs/TCA6408.h"

namespace OpenKNX
{
    namespace GPIO
    {
        class DriverTCA6408 : public Base
        {
          private:
            TCA6408* _tca = nullptr;
          public:
            DriverTCA6408(uint16_t i2cAddr, TwoWire* wire);
            virtual int init() override;
            virtual void GPIOpinMode(uint8_t pin, int mode, bool preset, int status) override;
            virtual void GPIOdigitalWrite(uint8_t pin, int status) override;
            virtual bool GPIOdigitalRead(uint8_t pin) override;
        };
    }
}