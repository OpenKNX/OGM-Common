#pragma once
#include "OpenKNX/Led/Base.h"

namespace OpenKNX
{
    namespace Led
    {
        class GPIO : public Base
        {
          private:
            volatile long _activeOn = HIGH;
            volatile long _pin = -1;
            volatile bool _isDimmable = true;
            volatile bool _isI2C = false; // Track if this is an I2C expander pin
            volatile bool _pendingI2CState = false; // Pending ON/OFF state for I2C
            volatile uint8_t _pendingI2CBrightness = 0; // Pending brightness value
            volatile bool _hasPendingI2C = false;   // Flag: I2C write pending (set by ISR, cleared by main)
            volatile uint8_t _pwmCycle = 0;         // PWM cycle counter (0-9 for 10% steps at 100Hz = 10Hz PWM)
            volatile bool _lastPwmState = false;    // Last PWM state (for change detection to reduce I2C writes)

            void writeLed(uint8_t brightness) override;

          public:
            GPIO(long pin = -1, long activeOn = HIGH, bool isDimmable = true);
            void init() override;
            void flushPendingI2C(); // Flush pending I2C writes - MUST be called from main loop, NOT ISR!

            long getPin() { return _pin; }
        };
    } // namespace Led
} // namespace OpenKNX