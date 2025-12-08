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

        GPIO_RGB::GPIO_RGB(long pin_r, long pin_g, long pin_b, long activeOn, Color rgb)
        {
            _pins[0] = pin_r;
            _pins[1] = pin_g;
            _pins[2] = pin_b;
            _activeOn = activeOn;
            _color[0] = (((uint32_t)rgb) >> 16) & 0xFF;
            _color[1] = (((uint32_t)rgb) >> 8) & 0xFF;
            _color[2] = ((uint32_t)rgb) & 0xFF;
            _colorDirty = true;
        }

        void GPIO_RGB::init()
        {
            // no valid pin
            if (_pins[0] < 0 || _pins[1] < 0 || _pins[2] < 0)
                return;

            _initialized = true;
            for (int i = 0; i < 3; i++)
            {
                pinMode(_pins[i], OUTPUT);
                digitalWrite(_pins[i], !_activeOn);
            }
            logDebugP("GPIO_RGB::init set pins %d %d %d to %d", _pins[0], _pins[1], _pins[2], !_activeOn);
        }

        /*
         * write led state based on bool and _brightness
         */
        void GPIO_RGB::writeLed(uint8_t brightness)
        {
            // no valid pin
            if (_pins[0] < 0 || _pins[1] < 0 || _pins[2] < 0)
                return;

            uint8_t calcBrightness = (uint32_t)brightness * _maxBrightness / 255;

            if (calcBrightness == _currentLedBrightness && !_colorDirty)
                return;

            uint8_t pwmValues[3];
            pwmValues[0] = ((uint32_t)_color[0] * calcBrightness * OpenKNX_LedColor_Calibration[0] / (255 * 255));
            pwmValues[1] = ((uint32_t)_color[1] * calcBrightness * OpenKNX_LedColor_Calibration[1] / (255 * 255));
            pwmValues[2] = ((uint32_t)_color[2] * calcBrightness * OpenKNX_LedColor_Calibration[2] / (255 * 255));
            for (int i = 0; i < 3; i++)
            {
                analogWrite(_pins[i], _activeOn ? pwmValues[i] : (255 - pwmValues[i]));
            }
            _currentLedBrightness = calcBrightness;
            _colorDirty = false;
        }

        void GPIO_RGB::color(uint8_t r, uint8_t g, uint8_t b)
        {
            _color[0] = r;
            _color[1] = g;
            _color[2] = b;
            _colorDirty = true;
            writeLed(_currentLedBrightness);
        }
    } // namespace Led
} // namespace OpenKNX