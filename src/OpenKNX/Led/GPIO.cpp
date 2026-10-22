#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/I2C/ScheduledWire.h"
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

#ifdef OPENKNX_I2C_SCHEDULER
            // the scheduler drives the PWM edges of expander pins on its bus
            uint8_t address, reg, bit;
            bool autoIncrement;
            if (_isI2C && openknx.gpio.outputRegister(_pin, address, reg, bit, autoIncrement))
                _scheduledLed = openknxScheduledWire.addLed(address, reg, bit, _activeOn == HIGH, autoIncrement);
#endif
        }

        void GPIO::writeLed(uint8_t brightness)
        {
            // do nothing if not initialized
            if (!_initialized) return;

            uint8_t calcBrightness = _isDimmable ? (uint32_t)brightness * _maxBrightness / 255 : brightness;

            // I2C expanders: software PWM driven by Manager's global cycle
            if (_isI2C)
            {
#ifdef OPENKNX_I2C_SCHEDULER
                if (_scheduledLed >= 0)
                {
                    _currentLedBrightness = calcBrightness;
                    openknxScheduledWire.setLed(_scheduledLed, calcBrightness);
                    return;
                }
#endif
                uint8_t currentCycle = Manager::getPwmCycle();
                uint8_t pwmSteps = Manager::getPwmSteps();

                uint8_t dutyCycle;
                if (calcBrightness == 0) dutyCycle = 0;
                else if (calcBrightness >= 255)
                    dutyCycle = pwmSteps;
                else
                    dutyCycle = (calcBrightness * pwmSteps + 127) / 255;

                const bool willBeOn = (currentCycle < dutyCycle);

                _currentLedBrightness = calcBrightness;
                if (willBeOn == _lastPwmState)
                    return;

                _lastPwmState = willBeOn;

                // tick sets the flag only; the flush context does the blocking I2C write
                _pendingI2CState = willBeOn;
                _hasPendingI2C = true;
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

        // Flush pending I2C write; flush context only (RP2040 main loop, ESP32 flush task), never from ISR
        void GPIO::flushPendingI2C()
        {
            if (!_isI2C || !_hasPendingI2C)
                return;

            // Clear before reading the state: a tick landing during the write re-flags its newer state.
            _hasPendingI2C = false;
            const bool state = _pendingI2CState;
            if (openknx.gpio.digitalWrite(_pin, state ? _activeOn : !_activeOn) != 0)
                _hasPendingI2C = true; // failed: retry on the next flush, no busy wait here
        }

        // Re-assert pin direction (OUTPUT) + level; recovers a PCA9557 CONFIG-register glitch
        // on the display-shared Wire1 that flips the pin to INPUT (LED dark, unrevivable by writes).
        // Called at a low rate from Manager::flushI2C() so the glitch self-heals without a reboot.
        void GPIO::reassertI2C()
        {
            if (!_isI2C || !_initialized)
                return;

            openknx.gpio.pinMode(_pin, OUTPUT); // direction first: the actual recovery
            openknx.gpio.digitalWrite(_pin, _lastPwmState ? _activeOn : !_activeOn);
        }
    } // namespace Led
} // namespace OpenKNX