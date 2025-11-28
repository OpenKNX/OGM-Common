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

            // Skip if already at target brightness AND no pending I2C write
            if (calcBrightness == _currentLedBrightness && !_hasPendingI2C)
                return;

            // For I2C expanders: Choose pattern based on compile-time config
            if (_isI2C)
            {
                // Software PWM simulation: Calculate duty cycle
                // 10 steps @ 100Hz timer = 10Hz PWM frequency (100ms full cycle)
                // Maps brightness 0-255 to duty cycle 0-10 (10% steps)
                _pwmCycle = (_pwmCycle + 1) % 10;
                
                // Map brightness to duty cycle (0-10 steps)
                uint8_t dutyCycle;
                if (calcBrightness == 0)
                    dutyCycle = 0;
                else if (calcBrightness >= 255)
                    dutyCycle = 10;
                else
                    dutyCycle = ((uint16_t)calcBrightness * 10 + 127) / 255; // 0-10 with rounding
                
                bool willBeOn = (_pwmCycle < dutyCycle); // ON if within duty cycle
                
                // **OPTIMIZATION**: Only write if state changed (reduces I2C load by 90%!)
                if (willBeOn == _lastPwmState && calcBrightness == _currentLedBrightness)
                    return; // No state change, skip write
                
                _lastPwmState = willBeOn; // Update state tracker
                
#if defined(OPENKNX_I2C_USE_PENDING_PATTERN)
                // PENDING PATTERN: Set flag, write in main loop
                _pendingI2CState = willBeOn;
                _pendingI2CBrightness = calcBrightness;
                _hasPendingI2C = true;
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

            // Direct GPIO write (with PWM support) - safe from ISR for non-I2C pins
            if (calcBrightness == 0)
                openknx.gpio.digitalWrite(_pin, !_activeOn);
            else if (calcBrightness == 255)
                openknx.gpio.digitalWrite(_pin, _activeOn);
            else
                analogWrite(_pin, _activeOn ? calcBrightness : (255 - calcBrightness));

            _currentLedBrightness = calcBrightness;
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