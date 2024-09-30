#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        void GPIO::init(long pin /* = -1 */, long activeOn /* = HIGH */)
        {
            // no valid pin
            if (pin < 0)
                return;

            _pin = pin;
            _activeOn = activeOn;

            pinMode(_pin, OUTPUT);
            digitalWrite(_pin, LOW);
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
                digitalWrite(_pin, _activeOn == HIGH ? true : false);

            else if (calcBrightness == 0)
                digitalWrite(_pin, _activeOn == HIGH ? false : true);

            else
                analogWrite(_pin, _activeOn == HIGH ? calcBrightness : (255 - calcBrightness));

            _currentLedBrightness = calcBrightness;
        }
    } // namespace Led
} // namespace OpenKNX