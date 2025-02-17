#include "DriverTCA6408.h"

namespace OpenKNX
{
    namespace GPIO
    {

        DriverTCA6408::DriverTCA6408(uint16_t i2cAddr, TwoWire* wire)
        {
            _tca = new TCA6408(i2cAddr, wire);
        }

        int DriverTCA6408::init()
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

        void DriverTCA6408::GPIOpinMode(uint8_t pin, int mode, bool preset, int status)
        {
            if(mode != INPUT && mode != OUTPUT || pin > 7)
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

        void DriverTCA6408::GPIOdigitalWrite(uint8_t pin, int status)
        {
            if(pin > 7)
            {
                // log some message
                return;
            }
            
            _tca->write1(pin, status);
        }

        bool DriverTCA6408::GPIOdigitalRead(uint8_t pin)
        {
            if(pin > 7)
            {
                // log some message
                return 0;
            }
            return _tca->read1(pin);
        }
    }
}