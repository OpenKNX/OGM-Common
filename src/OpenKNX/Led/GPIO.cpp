#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/Led/Manager.h"

namespace OpenKNX
{
    namespace Led
    {
        GPIO::GPIO(long pin /*= -1*/, long activeOn /*= HIGH*/, bool isDimmable /*= true*/, uint8_t maxBrightness /*= OPENKNX_LEDGPIO_MAX_BRIGHTNESS*/)
        {
            _pin = pin;
            _activeOn = activeOn;
            _isDimmable = isDimmable;
            _maxBrightness = maxBrightness;
        }

        void GPIO::init()
        {
            // no valid pin
            if (_pin < 0)
                return;

            _isI2C = (_pin > 0x00FF); // pins > 0xFF are I2C expander pins

            _initialized = true;
            if (!_isI2C && _isDimmable)
            {
#ifdef ANALOG
                pinMode(_pin, ANALOG);
#endif
                analogWrite(_pin, !_activeOn ? 255 : 0); // Start with OFF state
            }
            else
            {
                openknx.gpio.pinMode(_pin, OUTPUT);
                openknx.gpio.digitalWrite(_pin, !_activeOn);
            }
        }

        void GPIO::writeLed(uint8_t brightness)
        {
            // do nothing if not initialized
            if (!_initialized) return;

            uint8_t calcBrightness = _isDimmable ? (uint32_t)brightness * _maxBrightness / 255 : brightness;

            // I2C expanders: software PWM driven by Manager's global cycle
            if (_isI2C)
            {
                uint8_t currentCycle = Manager::getPwmCycle();
                uint8_t pwmSteps = Manager::getPwmSteps();

                uint8_t dutyCycle;
                if (calcBrightness == 0) dutyCycle = 0;
                else if (calcBrightness >= 255)
                    dutyCycle = pwmSteps;
                else
                    dutyCycle = (calcBrightness * pwmSteps + 127) / 255;

                const bool willBeOn = (currentCycle < dutyCycle);

                if (willBeOn == _lastPwmState && calcBrightness == _currentLedBrightness)
                    return;

                _lastPwmState = willBeOn;

                // ISR sets flag only; main loop does the blocking I2C write
                _pendingI2CState = willBeOn;
                _pendingI2CBrightness = calcBrightness;
                _hasPendingI2C = true;
                _currentLedBrightness = calcBrightness;
                return;
            }
            else
            {
                if (!_isDimmable)
                {
                    if (calcBrightness == 0)
                        openknx.gpio.digitalWrite(_pin, !_activeOn);
                    else if (calcBrightness == 255)
                        openknx.gpio.digitalWrite(_pin, _activeOn);
                }
                else
                    analogWrite(_pin, _activeOn ? calcBrightness : (255 - calcBrightness)); // invert for LOW active
                _currentLedBrightness = calcBrightness;
            }
        }

        // Flush pending I2C write; main loop only, never from ISR
        void GPIO::flushPendingI2C()
        {
            if (!_isI2C || !_hasPendingI2C)
                return;

            int result = -1;
            for (int retry = 0; retry < 10 && result != 0; retry++)
            {
                result = openknx.gpio.digitalWrite(_pin, _pendingI2CState ? _activeOn : !_activeOn);
                if (result != 0 && retry < 9)
                    delayMicroseconds(100);
            }

            if (result == 0)
            {
                _currentLedBrightness = _pendingI2CBrightness;
                _hasPendingI2C = false;
            }
            // still failing after retries: keep pending flag for next loop
        }

        // Re-assert pin direction (OUTPUT) + level; recovers a PCA9557 CONFIG-register glitch
        // on the display-shared Wire1 that flips the pin to INPUT (LED dark, unrevivable by writes).
        // Called at a low rate from Manager::loop() so the glitch self-heals without a reboot.
        void GPIO::reassertI2C()
        {
            if (!_isI2C || !_initialized)
                return;

            openknx.gpio.pinMode(_pin, OUTPUT); // direction first: the actual recovery
            openknx.gpio.digitalWrite(_pin, _lastPwmState ? _activeOn : !_activeOn);
        }
    } // namespace Led
} // namespace OpenKNX