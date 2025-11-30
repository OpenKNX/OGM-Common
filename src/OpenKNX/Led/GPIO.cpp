#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        GPIO::GPIO(long pin /*= -1*/, long activeOn /*= HIGH*/, bool isDimmable /*= true*/)
        {
            _pin = pin;
            _activeOn = activeOn;
            _isDimmable = isDimmable;
        }

        void GPIO::init()
        {
            // no valid pin
            if (_pin < 0)
                return;

            if (_pin > 0x00FF & _isDimmable)
            {
                logErrorP("Led::GPIO does not support dimmable LED on I2C expander");
                _isDimmable = false;
            }

            _initialized = true;
            openknx.gpio.pinMode(_pin, OUTPUT);
            openknx.gpio.digitalWrite(_pin, !_activeOn);
        }

        /*
         * write led state based on bool and _brightness
         */
        void GPIO::writeLed(uint8_t brightness)
        {
            // do nothing if not initialized
            if (_initialized < 0) return;

            uint8_t calcBrightness = (uint32_t)brightness * _maxBrightness / 255;
            if (calcBrightness == _currentLedBrightness)
                return;

            if (_isDimmable)
            {
                analogWrite(_pin, _activeOn ? calcBrightness : (255 - calcBrightness));
                _currentLedBrightness = calcBrightness;
            }
            else
            {
                if (calcBrightness)
                {
                    openknx.gpio.digitalWrite(_pin, _activeOn);
                }
                else
                {
                    openknx.gpio.digitalWrite(_pin, !_activeOn);
                }

                _currentLedBrightness = calcBrightness;
            }
        }
    } // namespace Led
} // namespace OpenKNX