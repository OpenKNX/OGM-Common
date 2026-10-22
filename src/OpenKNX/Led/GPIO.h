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
            volatile bool _hasPendingI2C = false;       // I2C write pending (set by tick, cleared by flush)
            volatile bool _lastPwmState = false;        // last PWM state, for change detection
#ifdef OPENKNX_I2C_SCHEDULER
            int8_t _scheduledLed = -1; // LED index in the I2C scheduler; -1 = soft-PWM via pending flags
#endif

            void writeLed(uint8_t brightness) override;

          public:
            GPIO(long pin = -1, long activeOn = HIGH, bool isDimmable = true, uint8_t maxBrightness = OPENKNX_LEDGPIO_MAX_BRIGHTNESS);
            void init() override;
            void flushPendingI2C(); // flush pending I2C write; flush context only, never from ISR
            void reassertI2C();     // re-assert expander pin direction+level; recovers CONFIG-register glitch, flush context only

            long getPin() { return _pin; }
            bool isI2C() const { return _isI2C; }
#ifdef OPENKNX_I2C_SCHEDULER
            bool isScheduled() const { return _scheduledLed >= 0; }
#endif
            GPIO* asGPIO() override { return this; }
        };
    } // namespace Led
} // namespace OpenKNX