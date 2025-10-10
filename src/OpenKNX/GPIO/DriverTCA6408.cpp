#include "DriverTCA6408.h"

namespace OpenKNX
{
    namespace GPIO
    {

        DriverTCA6408::DriverTCA6408(uint16_t i2cAddr, TwoWire* wire, uint8_t interruptPin)
        {
            _tca = new TCA6408(i2cAddr, wire);
            if (interruptPin != 0xFF)
            {
                attachInterrupt(digitalPinToInterrupt(interruptPin), []() {
                    // not implemented
                    // poll data of the expander
                    // compare with the last state
                    // call the callback function, if present
                },
                                FALLING); // INT pin for TCA6408 is active low
            }
        }

        int DriverTCA6408::init()
        {
            if (_tca->begin())
            {
                _initState = GPIOInitState::OK;
            }
            else
            {
                _initState = GPIOInitState::Failed;
            }
            return static_cast<int>(_initState);
        }

        void DriverTCA6408::GPIOpinMode(uint8_t pin, int mode, bool preset, int status)
        {
            if (mode != INPUT && mode != OUTPUT || pin > 7)
            {
                // log some message
                return;
            }

            if (preset)
            {
                _tca->write1(pin, status);
            }
            _tca->pinMode1(pin, mode);
        }

        void DriverTCA6408::GPIOdigitalWrite(uint8_t pin, int status)
        {
            if (pin > 7)
            {
                // log some message
                return;
            }

            _tca->write1(pin, status);
        }

        bool DriverTCA6408::GPIOdigitalRead(uint8_t pin)
        {
            if (pin > 7)
            {
                // log some message
                return 0;
            }
            return _tca->read1(pin);
        }
    } // namespace GPIO
} // namespace OpenKNX