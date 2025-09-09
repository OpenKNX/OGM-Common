#include "OpenKNX/Led/GPIO_RGB.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        GPIO_RGB::GPIO_RGB(long pin_r, long pin_g, long pin_b, long activeOn, uint8_t r, uint8_t g, uint8_t b)
        {
            _pins[0] = pin_r;
            _pins[1] = pin_g;
            _pins[2] = pin_b;
            _activeOn = activeOn;
            _color[0] = r;
            _color[1] = g;
            _color[2] = b;
            _colorDirty = true;
        }

        void GPIO_RGB::init()
        {
            // no valid pin
            if (_pins[0] < 0 || _pins[1] < 0 || _pins[2] < 0)
                return;

            _initialized = true;
            for(int i = 0; i < 3; i++)
            {
                pinMode(_pins[i], OUTPUT);
                digitalWrite(_pins[i], !_activeOn);
            }
        }

        /*
         * write led state based on bool and _brightness
         */
        void GPIO_RGB::writeLed(uint8_t brightness)
        {
            // no valid pin
            if (_pins[0] < 0 || _pins[1] < 0 || _pins[2] < 0)
                return;

            uint8_t calcBrightness = (uint32_t)brightness * _maxBrightness / 100;

            if (calcBrightness == _currentLedBrightness && !_colorDirty)
                return;

            uint8_t pwmValues[3];
            for(int i = 0; i < 3; i++)
            {
                pwmValues[i] = ((uint32_t)_color[i] * calcBrightness / 256);

                if(pwmValues[i] == 255)
                {
                    pinMode(_pins[i], OUTPUT);
                    digitalWrite(_pins[i], _activeOn);
                }
                else if(pwmValues[i] == 0)
                {
                    pinMode(_pins[i], OUTPUT);
                    digitalWrite(_pins[i], !_activeOn);
                }
                else
                {
                    analogWrite(_pins[i], _activeOn ? pwmValues[i] : (255 - pwmValues[i]));
                }
            }
            _currentLedBrightness = calcBrightness;
        }

        void GPIO_RGB::setColor(uint8_t r, uint8_t g, uint8_t b)
        {
            _color[0] = r;
            _color[1] = g;
            _color[2] = b;
            _colorDirty = true;
            writeLed(_currentLedBrightness);
        }
    } // namespace Led
} // namespace OpenKNX