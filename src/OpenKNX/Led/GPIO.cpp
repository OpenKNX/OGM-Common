#include "OpenKNX/Led/GPIO.h"
#include "OpenKNX/Led/Manager.h"
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

            _isI2C = (_pin > 0x00FF); // Check if this is an I2C expander pin (pins > 0xFF are I2C)
            // I2C LEDs now support software PWM simulation (via pending pattern)

            _initialized = true;
            openknx.gpio.pinMode(_pin, OUTPUT);
            openknx.gpio.digitalWrite(_pin, !_activeOn);
        }

        void GPIO::writeLed(uint8_t brightness)
        {
            // do nothing if not initialized
            if (!_initialized) return;

            uint8_t calcBrightness = _isDimmable ? (uint32_t)brightness * _maxBrightness / 255 : brightness;

            // For I2C expanders: Use Manager's global PWM cycle
            if (_isI2C)
            {
                // Get current cycle from Manager (updated every timer interrupt)
                uint8_t currentCycle = Manager::getPwmCycle();
                uint8_t pwmSteps = Manager::getPwmSteps();
                
                // Map brightness to duty cycle (0-pwmSteps)
                uint8_t dutyCycle;
                if (calcBrightness == 0) dutyCycle = 0;
                else if (calcBrightness >= 255) dutyCycle = pwmSteps;
                else dutyCycle = (calcBrightness * pwmSteps + 127) / 255; // Scale to 0-pwmSteps
                
                // Determine desired state based on PWM cycle
                const bool willBeOn = (currentCycle < dutyCycle);
                
                // Skip write if no change (brightness same AND state same)
                if (willBeOn == _lastPwmState && calcBrightness == _currentLedBrightness)
                    return;
                
                _lastPwmState = willBeOn; // Update state tracker
                
#if defined(OPENKNX_I2C_USE_PENDING_PATTERN)
                // PENDING PATTERN: Set flag, write in main loop
                _pendingI2CState = willBeOn;
                _pendingI2CBrightness = calcBrightness;
                _hasPendingI2C = true;
                _currentLedBrightness = calcBrightness;
                return;
#elif defined(OPENKNX_I2C_USE_ASYNC_QUEUE)
                // ASYNC_QUEUE: Direct write, queued by I2C layer
                openknx.gpio.digitalWrite(_pin, willBeOn ? _activeOn : !_activeOn);
                _currentLedBrightness = calcBrightness;
                return;
#else
                // SPINLOCK PATTERN: Try direct write first, fallback to pending on failure
                int result = openknx.gpio.digitalWrite(_pin, willBeOn ? _activeOn : !_activeOn);
                if (result == 0)
                {
                    // I2C write successful - update cached state
                    _currentLedBrightness = calcBrightness;
                    _hasPendingI2C = false; // Clear any pending write
                }
                else
                {
                    // I2C busy - set pending flag for main loop to handle
                    _pendingI2CState = willBeOn;
                    _pendingI2CBrightness = calcBrightness;
                    _hasPendingI2C = true;
                    // Don't update _currentLedBrightness yet - will be updated when pending write succeeds
                }
                return;
#endif
            }
            else
            {
                // Direct GPIO write (non-I2C pin)
                if (calcBrightness == 0)
                    openknx.gpio.digitalWrite(_pin, !_activeOn);
                else if (calcBrightness == 255)
                    openknx.gpio.digitalWrite(_pin, _activeOn);
                else
                    analogWrite(_pin, _activeOn ? calcBrightness : (255 - calcBrightness)); // Invert for LOW active  
                _currentLedBrightness = calcBrightness;
            }
        }

        // Flush pending I2C writes - called from main loop, NOT from ISR!
        void GPIO::flushPendingI2C()
        {
            if (!_isI2C || !_hasPendingI2C)
                return;

            // Main loop context - retry with small delays until success
            // (safe to wait here, we're not in ISR)
            int result = -1;
            for (int retry = 0; retry < 10 && result != 0; retry++)
            {
                result = openknx.gpio.digitalWrite(_pin, _pendingI2CState ? _activeOn : !_activeOn);
                if (result != 0 && retry < 9)
                    delayMicroseconds(100); // Wait 100µs before retry
            }
            
            if (result == 0)
            {
                // Success - update cached brightness to the originally requested value
                _currentLedBrightness = _pendingI2CBrightness;
                _hasPendingI2C = false;
            }
            // If still fails after retries, keep pending flag for next loop cycle
        }
    } // namespace Led
} // namespace OpenKNX