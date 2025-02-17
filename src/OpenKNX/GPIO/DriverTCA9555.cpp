#include "DriverTCA9555.h"

namespace OpenKNX
{
    namespace GPIO
    {

        DriverTCA9555::DriverTCA9555(uint16_t i2cAddr, TwoWire* wire)
        {
            _tca = new TCA9555(i2cAddr, wire);
        }

        int DriverTCA9555::init()
        {
            if (_tca->begin())
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }

        void DriverTCA9555::GPIOpinMode(uint8_t pin, int mode, bool preset, int status)
        {
            if(mode != INPUT && mode != OUTPUT || pin > 15)
            {
                // log some message
                return;
            }

            if(preset)
            {
                _tca->write1(pin, status);
            }
            _tca->pinMode1(pin, mode);
        }

        void DriverTCA9555::GPIOdigitalWrite(uint8_t pin, int status)
        {
            if(pin > 15)
            {
                // log some message
                return;
            }
            
            _tca->write1(pin, status);
        }

        bool DriverTCA9555::GPIOdigitalRead(uint8_t pin)
        {
            if(pin > 15)
            {
                // log some message
                return 0;
            }
            return _tca->read1(pin);
        }
    }
}