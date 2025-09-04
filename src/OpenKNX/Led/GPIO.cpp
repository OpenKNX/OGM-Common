#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        GPIO::GPIO(long pin /*= -1*/, long activeOn /*= HIGH*/)
        {
            _pin = pin;
            _activeOn = activeOn;
        }

        void GPIO::init()
        {
            // no valid pin
            if (_pin < 0)
                return;
            
            _initialized = true;
            pinMode(_pin, OUTPUT);
            digitalWrite(_pin, !_activeOn);
        }

        /*
         * write led state based on bool and _brightness
         */
        void GPIO::writeLed(uint8_t brightness)
        {
            // no valid pin
            if (_pin < 0) return;

            uint8_t calcBrightness = (uint32_t)brightness * _maxBrightness / 100;

            if (calcBrightness == _currentLedBrightness)
                return;

            // Need to reset pinMode after using analogWrite
            if (_currentLedBrightness != 0 || _currentLedBrightness != 255)
                pinMode(_pin, OUTPUT);

            if (calcBrightness == 255)
                digitalWrite(_pin, _activeOn);

            else if (calcBrightness == 0)
                digitalWrite(_pin, !_activeOn);

            else
                analogWrite(_pin, _activeOn ? calcBrightness : (255 - calcBrightness));

            _currentLedBrightness = calcBrightness;
        }
    } // namespace Led
} // namespace OpenKNX