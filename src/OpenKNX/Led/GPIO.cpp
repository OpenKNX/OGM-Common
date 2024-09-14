#include "OpenKNX/Led/GPIO.h"

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
        void GPIO::writeLed(uint16_t brightness)
        {
            // no valid pin
            if (_pin < 0) return;

            if (brightness == _currentLedBrightness)
                return;

            // Need to reset pinMode after using analogWrite
            if (_currentLedBrightness != 0 || _currentLedBrightness != 65535)
                pinMode(_pin, OUTPUT);

            // logTraceP("==== > %i -> %i\n", _pin, brightness);
            if (brightness == 65535)
                digitalWrite(_pin, _activeOn == HIGH ? true : false);

            else if (brightness == 0)
                digitalWrite(_pin, _activeOn == HIGH ? false : true);

            else
                analogWrite(_pin, (_activeOn == HIGH ? brightness : (65535 - brightness) / 256));

            _currentLedBrightness = brightness;
        }
    } // namespace Led
} // namespace OpenKNX