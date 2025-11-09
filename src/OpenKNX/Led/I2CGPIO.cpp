#include "OpenKNX/Led/I2CGPIO.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        I2CGPIO::I2CGPIO(long pin /*= -1*/, long activeOn /*= HIGH*/)
        {
            _pin = pin;
            _activeOn = activeOn;
        }

        void I2CGPIO::init()
        {
            // no valid pin
            if (_pin < 0)
                return;

            _initialized = true;
            openknx.gpio.pinMode(_pin, OUTPUT);
            openknx.gpio.digitalWrite(_pin, !_activeOn);
        }

        /*
         * write led state based on bool and _brightness
         */
        void I2CGPIO::writeLed(uint8_t brightness)
        {
            // do nothing if not initialized
            if (_initialized < 0) return;

            if (brightness == _currentLedBrightness)
                return;

            if (!brightness)
            {
                openknx.gpio.digitalWrite(_pin, _activeOn);
            }
            else
            {
                openknx.gpio.digitalWrite(_pin, !_activeOn);
            }

            _currentLedBrightness = brightness;
        }

        bool I2CGPIO::isDimmable()
        {
            return false;
        }
    } // namespace Led
} // namespace OpenKNX