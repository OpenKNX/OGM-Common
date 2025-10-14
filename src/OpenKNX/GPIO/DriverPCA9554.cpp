#include "DriverPCA9554.h"

namespace OpenKNX
{
    namespace GPIO
    {

        DriverPCA9554::DriverPCA9554(uint8_t i2cAddr, TwoWire* wire)
        {
            _pca = new PCA95XX(wire, i2cAddr, PCA95XX_PCA9554);
        }

        DriverPCA9554::~DriverPCA9554()
        {
            delete _pca;
        }

        int DriverPCA9554::init()
        {
            if (_pca->begin())
            {
                _initState = GPIOInitState::OK;
            }
            else
            {
                _initState = GPIOInitState::Failed;
            }
            return static_cast<int>(_initState);
        }

        void DriverPCA9554::GPIOpinMode(uint8_t pin, int mode, bool preset, int status)
        {
            if (pin > 7 || (mode != INPUT && mode != OUTPUT))
            {
                return;
            }

            if (preset)
            {
                _pca->digitalWrite(pin, status);
            }

            if (mode == INPUT)
            {
                _pca->pinMode(pin, INPUT);
            }
            else
            {
                _pca->pinMode(pin, OUTPUT);
            }
        }

        // Writes a digital value (HIGH/LOW) to the specified pin
        void DriverPCA9554::GPIOdigitalWrite(uint8_t pin, int status)
        {
            if (pin > 7)
            {
                return;
            }
            _pca->digitalWrite(pin, status);
        }

        bool DriverPCA9554::GPIOdigitalRead(uint8_t pin)
        {
            if (pin > 7)
            {
                return false;
            }
            return _pca->digitalRead(pin);
        }
    } // namespace GPIO
} // namespace OpenKNX