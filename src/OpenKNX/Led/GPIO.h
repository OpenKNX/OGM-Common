#pragma once
#include "OpenKNX/Led/Base.h"

#ifndef OPENKNX_LEDGPIO_MAX_BRIGHTNESS
    #define OPENKNX_LEDGPIO_MAX_BRIGHTNESS 255
#endif

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
            volatile bool _isI2C = false;               // I2C expander pin
            volatile bool _pendingI2CState = false;     // pending ON/OFF state
            volatile uint8_t _pendingI2CBrightness = 0; // pending brightness
            volatile bool _hasPendingI2C = false;       // I2C write pending (set by ISR, cleared by main)
            volatile bool _lastPwmState = false;        // last PWM state, for change detection

            void writeLed(uint8_t brightness) override;

          public:
            GPIO(long pin = -1, long activeOn = HIGH, bool isDimmable = true, uint8_t maxBrightness = OPENKNX_LEDGPIO_MAX_BRIGHTNESS);
            void init() override;
            void flushPendingI2C(); // flush pending I2C write; main loop only, never from ISR
            void reassertI2C();     // re-assert expander pin direction+level; recovers CONFIG-register glitch, main loop only

            long getPin() { return _pin; }
            GPIO* asGPIO() override { return this; }
        };
    } // namespace Led
} // namespace OpenKNX