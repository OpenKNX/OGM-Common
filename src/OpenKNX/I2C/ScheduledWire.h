#pragma once
/**
 * @file        ScheduledWire.h
 * @brief       Time-slotted owner of one I2C bus: soft-PWM edges for expander LEDs at fixed times, all other transfers in the gaps
 * @version     0.0.1
 * @date        2026-10-23
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Opt-in per board with OPENKNX_I2C_SCHEDULER. The object is a TwoWire, so drivers on the bus keep their
 * synchronous Wire calls; a transfer starts only when it ends before the next LED edge. Edges run from an
 * alarm IRQ on RP2040/RP2350 and from a bus task on ESP32. All bus users must get this object, not Wire1.
 * Use it from task / main-loop context only, never from an ISR or the LED tick. requestFrom(..., false)
 * (bus kept without STOP) is not supported: the next LED edge may start in between.
 * endTransmission(false) is sent together with the following requestFrom(). If beginTransmission() comes
 * instead, RP2040 sends the pending write with STOP first; ESP32 drops it, like its TwoWire.
 * Without a dimmed LED other transfers do not wait for the period start, so an on/off change may come
 * up to one transfer burst later (a display page, about 5 ms at 400 kHz).
 */

#include "OpenKNX/defines.h" // board config decides OPENKNX_I2C_SCHEDULER

#ifdef OPENKNX_I2C_SCHEDULER
    #include <Arduino.h>
    #include <Wire.h>
    #ifdef ARDUINO_ARCH_RP2040
        #include "hardware/i2c.h"
        #include "hardware/sync.h"
        #include "pico/time.h"
    #endif

    #if !defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_ESP32)
        #error "OPENKNX_I2C_SCHEDULER is only available on RP2040/RP2350 and ESP32"
    #endif
    #if defined(OPENKNX_WIRE_PIO) || defined(OPENKNX_WIRE1_PIO)
        #error "OPENKNX_I2C_SCHEDULER cannot be combined with PIO I2C"
    #endif

    #ifndef OPENKNX_I2C_SCHEDULER_PWM_HZ
        #define OPENKNX_I2C_SCHEDULER_PWM_HZ 200 // soft-PWM period rate
    #endif
    #ifndef OPENKNX_I2C_SCHEDULER_MAX_LEDS
        #define OPENKNX_I2C_SCHEDULER_MAX_LEDS 4
    #endif
    #ifndef OPENKNX_I2C_SCHEDULER_MAX_PORTS
        #define OPENKNX_I2C_SCHEDULER_MAX_PORTS 2 // expander output registers carrying LEDs
    #endif
    #ifndef OPENKNX_I2C_SCHEDULER_OVERHEAD_US
        #ifdef ARDUINO_ARCH_ESP32
            #define OPENKNX_I2C_SCHEDULER_OVERHEAD_US 120 // driver time per transfer on top of the wire time
        #else
            #define OPENKNX_I2C_SCHEDULER_OVERHEAD_US 20
        #endif
    #endif
    #ifndef OPENKNX_I2C_SCHEDULER_BUS
        #ifdef ARDUINO_ARCH_ESP32
            #define OPENKNX_I2C_SCHEDULER_BUS 1 // I2C port number
        #else
            #define OPENKNX_I2C_SCHEDULER_BUS ((OPENKNX_GPIO_SDA >> 1) & 1) // I2C block of the SDA pin
        #endif
    #endif

namespace OpenKNX
{
    namespace I2C
    {
        class ScheduledWire : public TwoWire
        {
          public:
            ScheduledWire();

            using TwoWire::requestFrom;
            using TwoWire::write;

            uint8_t endTransmission(bool stopBit) override;
            uint8_t endTransmission() override { return endTransmission(true); }
            size_t requestFrom(uint8_t address, size_t len, bool stopBit) override;
            size_t requestFrom(uint8_t address, size_t len) override { return requestFrom(address, len, true); }
    #ifdef ARDUINO_ARCH_ESP32
            bool setClock(uint32_t freq) override;
    #else
            void beginTransmission(uint8_t address) override;
            size_t write(uint8_t data) override;
            void setClock(uint32_t freqHz) override;
    #endif

            /**
             * @brief Register an expander output bit as soft-PWM LED. Returns the LED index or -1.
             * @note Starts the PWM engine on the first LED. Main loop only.
             */
            int8_t addLed(uint8_t address, uint8_t reg, uint8_t bit, bool activeHigh, bool autoIncrement);

            /**
             * @brief Set LED brightness 0..255; applied from the next PWM period. Safe from the LED tick.
             */
            inline void setLed(int8_t led, uint8_t brightness)
            {
                if (led >= 0 && led < _ledCount)
                    _leds[led].brightness = brightness;
            }

            /**
             * @brief Run the due PWM edges. Called from the alarm IRQ / bus task; false while a transfer holds the bus.
             */
            bool runEvents();
            inline uint32_t nextEventUs() const { return _nextEventUs; }
            inline uint32_t edgeUs() const { return _edgeUs; } // shortest pulse
            static constexpr uint32_t periodUs() { return 1000000UL / OPENKNX_I2C_SCHEDULER_PWM_HZ; }

    #ifdef OPENKNX_CONSOLE_DEV
            /// @brief Counters for the console stress test (leds pwm status).
            struct Stats
            {
                uint32_t events;         // PWM events run
                uint32_t late;           // events run more than 50 us after their time
                uint32_t maxLateUs;
                uint32_t busyRetries;    // events that found a transfer on the bus
                uint32_t edgeErrors;     // failed LED writes
                uint32_t transfers;      // driver transfers
                uint32_t maxWaitUs;      // longest wait of a transfer for its gap
                uint32_t forced;         // transfers that took the bus after one period
                uint32_t transferErrors; // failed or short driver transfers
                uint32_t stressReads;    // bus stress: expander reads
                uint32_t stressErrors;
            };
            inline const Stats& stats() const { return _stats; }
            void resetStats(); // owns the bus meanwhile, so no counter update is lost
            // late/maxLateUs count only events that held transfers back; transfers/maxWaitUs/forced only driver transfers
            /// @brief Bus stress: loop() reads the first LED expander's register 0 several times per call.
            inline void stressBus(bool on) { _stressBus = on; }
            inline bool stressingBus() const { return _stressBus; }
            void loop();
        #ifdef ARDUINO_ARCH_ESP32
            uint32_t busTaskStackFree() const; // bytes never used on the bus task stack
        #endif
    #endif

          private:
            struct Port
            {
                uint8_t address;
                uint8_t reg;
                bool autoIncrement;  // register pointer advances per data byte (TCA9555)
                uint8_t ledMask;     // bits driven by the PWM engine
                uint8_t activeLowMask;
                uint8_t other;       // last value written by a driver; source of the non-LED bits
                uint8_t onMask;      // LEDs currently on
                uint8_t written;     // value on the chip
                uint8_t pending;     // merged value of a driver write in flight
                bool writtenValid;
                uint8_t failures;    // failed LED writes in a row
                bool holding;        // LED writes paused after repeated errors ...
                uint32_t holdUntilUs; // ... until then
            };
            struct Led
            {
                uint8_t port;
                uint8_t mask;
                volatile uint8_t brightness;
            };
            struct Off
            {
                uint16_t atUs; // offset from the period start
                uint8_t port;
                uint8_t mask;
            };

            Port _ports[OPENKNX_I2C_SCHEDULER_MAX_PORTS] = {};
            Led _leds[OPENKNX_I2C_SCHEDULER_MAX_LEDS] = {};
            Off _off[OPENKNX_I2C_SCHEDULER_MAX_LEDS] = {};
            volatile uint8_t _portCount = 0;
            volatile uint8_t _ledCount = 0;
            uint8_t _offCount = 0;
            uint8_t _offIndex = 0;

    #ifdef OPENKNX_CONSOLE_DEV
            Stats _stats = {};
            volatile bool _stressBus = false;
    #endif
            uint32_t _clkHz = 100000;
            uint32_t _edgeUs = 0; // one LED write incl. margin; shortest pulse and edge spacing
            uint32_t _periodStartUs = 0;
            bool _periodValid = false;
            volatile uint32_t _nextEventUs = 0;
            volatile bool _nextEventBlocks = true; // the next event writes LEDs; a period start without dimmed LEDs does not wait
            volatile bool _running = false;
            volatile bool _busy = false;
            volatile bool _eventWaiting = false;
    #ifdef ARDUINO_ARCH_ESP32
            portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    #else
            i2c_inst_t* _inst;
            uint8_t _txAddress = 0;
            uint8_t _txHead[3] = {}; // first bytes of a write; kept here until the end so the output-register merge can change them
            size_t _txLen = 0;
            bool _deferredWrite = false; // endTransmission(false) waiting for its requestFrom()
            bool _inTransfer = false;    // TwoWire's timeout recovery calls setClock() from inside a transfer
            bool _txOpen = false;        // a transmission is open (TwoWire's _txBegun is private)
            alarm_id_t _alarmId = 0;     // edge alarm; re-armed by release() when an edge waited for the bus
            void forwardHead();
            uint8_t transmit();
    #endif

    #ifdef ARDUINO_ARCH_ESP32
            inline uint32_t schedLock()
            {
                portENTER_CRITICAL(&_mux);
                return 0;
            }
            inline void schedUnlock(uint32_t) { portEXIT_CRITICAL(&_mux); }
    #else
            inline uint32_t schedLock() { return save_and_disable_interrupts(); }
            inline void schedUnlock(uint32_t state) { restore_interrupts(state); }
    #endif
            uint32_t transferUs(size_t writeBytes, size_t readBytes) const;
            void updateTiming();
            void grant(uint32_t durationUs, bool driverTransfer = true);
            void release();
            uint8_t mergeOutputWrite(uint8_t address, uint8_t* data, size_t len);
            void commitOutputWrite(uint8_t ports, bool ok);
            void beginPeriod(uint32_t now);
            void writePorts(uint32_t now);
            bool writeRaw(uint8_t address, uint8_t reg, uint8_t value);
            bool start();
        };
    } // namespace I2C
} // namespace OpenKNX

extern OpenKNX::I2C::ScheduledWire openknxScheduledWire;

#endif // OPENKNX_I2C_SCHEDULER
