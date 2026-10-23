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
 * synchronous Wire calls; a transfer starts only when it ends before the next LED edge. All bus users must
 * get this object, not Wire1. Use it from task / main-loop context, one caller at a time, never from an ISR
 * or the LED tick. requestFrom(..., false) (bus kept without STOP) is not supported: the next LED edge may
 * start in between. endTransmission(false) is sent together with the following requestFrom(). If
 * beginTransmission() comes instead, RP2040 sends the pending write with STOP first; ESP32 drops it, like its
 * TwoWire. Without a dimmed LED other transfers do not wait for the period start, so an on/off change may
 * come up to one transfer burst later.
 *
 * RP2040/RP2350 (Arm cores): transfers run from the I2C IRQ through the hardware FIFOs, edges from an own
 * alarm, display chunks through a write queue, expander inputs are read in the gaps (ScheduledWireRp2040.cpp).
 * ESP32: edges from a bus task, transfers through the ESP32 TwoWire (ScheduledWire.cpp).
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
    #ifdef ARDUINO_ARCH_RP2040
        #ifdef __riscv
            #error "OPENKNX_I2C_SCHEDULER on RP2350 needs the Arm cores"
        #endif
        #define SCHED_RAM(func) __not_in_flash_func(func) // engine path under the lock: no XIP cache misses with IRQs off
        #ifndef OPENKNX_I2C_SCHEDULER_MAX_INPUTS
            #define OPENKNX_I2C_SCHEDULER_MAX_INPUTS 2 // expanders whose inputs are read in the gaps
        #endif
        #ifndef OPENKNX_I2C_SCHEDULER_QUEUE
            #define OPENKNX_I2C_SCHEDULER_QUEUE 8 // queued writes (display chunks)
        #endif
        #ifndef OPENKNX_I2C_SCHEDULER_RISE_NS
            #define OPENKNX_I2C_SCHEDULER_RISE_NS 300 // SCL rise time per bit (fast-mode maximum); depends on pull-ups and bus load
        #endif
    #else
        #define SCHED_RAM(func) func
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
            using TwoWire::begin;
            void begin() override;
            void end() override;
            int available() override;
            int read() override;
            int peek() override;

            /**
             * @brief Read input register 0 of an 8-bit expander every 10 ms in the bus gaps. Main loop, after begin().
             * @param commandAfterRead write command byte 0x01 after each read (PCA9554 interrupt errata)
             */
            void watchInputs(uint8_t address, bool commandAfterRead);
            /// @brief Inputs of a watched expander from the last read: at most 50 ms old, or the last valid value while reads fail.
            bool cachedInputs(uint8_t address, uint16_t& value) const;

            /**
             * @brief Queue a write of one or two segments (first 1..8, second 0..33 bytes) without waiting; the data is copied.
             * @note Sent in the bus gaps, each segment with STOP, the second right after the first: nothing else goes to
             *       the address in between. False when the queue is full. Main loop.
             */
            bool queueWrite(uint8_t address, const uint8_t* first, uint8_t firstLen, const uint8_t* second, uint8_t secondLen, uint32_t tag);
            /// @brief Take back the oldest finished queued write: its tag and whether all of it was sent. False if none is finished.
            /// @note One queue for all users: a second user would get the tags of the first. Today only the display queues.
            bool queueResult(uint32_t& tag, bool& ok);
            /// @brief The engine runs; queued writes are sent.
            inline bool queueReady() const { return _engine && _hwUp; }
            void onIrq(); // I2C IRQ
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
                if (led < 0 || led >= _ledCount)
                    return;
    #ifdef ARDUINO_ARCH_ESP32
                portENTER_CRITICAL(&_mux); // runEvents() decides the next start from the brightness on another core
    #endif
                _leds[led].brightness = brightness;
                if (hasOffEdge(brightness))
                    _nextEventBlocks = true; // the next period start plans an off-edge: transfers leave it free from now on
    #ifdef ARDUINO_ARCH_ESP32
                portEXIT_CRITICAL(&_mux);
    #endif
            }

            /**
             * @brief Run the due PWM edges. RP2040: edge alarm, always true; ESP32: bus task, false while a transfer holds the bus.
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
                uint64_t waitUs;         // caller time spent waiting for a gap (driver transfers)
                uint64_t transferUs;     // caller time from the gap to the end of the transfer
                uint64_t edgeWriteUs;    // RP2040: time in the engine IRQs (I2C IRQ, edge alarm); ESP32: LED edge writes in the bus task
                uint32_t sinceMs;        // millis() of the last reset; base of the shares above
        #ifdef ARDUINO_ARCH_RP2040
                uint32_t recoveries;     // stuck transfers aborted, bus reset when the abort did not end it
                uint32_t inputReads;     // expander input reads of the input cache
                uint32_t queued;         // queued writes finished (display chunks), failed ones included
                uint32_t queueErrors;    // queued writes that failed
                uint32_t queueFull;      // queueWrite() calls refused, queue full
        #endif
            };
            Stats stats(); // copy taken while owning the bus, so no counter is read half-updated; main loop only
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
            static constexpr uint32_t kMarginUs = 20;   // gap between a transfer end and the next edge
            static constexpr uint32_t kSlackUs = 10;    // edges this close are run in one event
    #ifdef OPENKNX_CONSOLE_DEV
            static constexpr uint32_t kLateUs = 50;     // stats: an event later than this counts as late
            static constexpr uint8_t kStressReads = 4;  // bus stress: expander reads per loop()
    #endif
            static constexpr uint32_t kHoldUs = 100000; // LED writes paused after kHoldFailures failed writes in a row
            static constexpr uint8_t kHoldFailures = 3;

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
            uint32_t _grantUs = 0;      // start of the counted transfer
            bool _grantCounted = false; // release() adds the transfer time
            bool _inStress = false;     // bus-stress reads are not counted as driver transfers
    #endif
            uint32_t _clkHz = 100000;
    #ifdef ARDUINO_ARCH_RP2040
            uint32_t _bitNs = 10300; // SCL period on the wire; set by updateTiming()
    #endif
            uint32_t _edgeUs = 0; // one LED write incl. margin; shortest pulse and edge spacing
            uint32_t _periodStartUs = 0;
            bool _periodValid = false;
            volatile uint32_t _nextEventUs = 0;
            volatile bool _nextEventBlocks = true; // the next event writes LEDs; a period start without dimmed LEDs does not wait
            bool _startBlocks = true;              // _nextEventBlocks as the last event stored it (setLed() may raise the flag later)
            volatile bool _running = false;
    #ifdef ARDUINO_ARCH_ESP32
            volatile bool _busy = false;
            volatile bool _eventWaiting = false;
            portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    #else
            i2c_inst_t* _inst;
            uint8_t _txAddress = 0;
            uint8_t _txHead[33] = {}; // a whole write up to a display chunk (data-mode byte + 32 columns); longer ones go to TwoWire
            size_t _txLen = 0;
            bool _deferredWrite = false; // endTransmission(false) waiting for its requestFrom()
            bool _inTransfer = false;    // TwoWire's timeout recovery calls setClock() from inside a transfer
            bool _txOpen = false;        // a transmission is open (TwoWire's _txBegun is private)
            alarm_id_t _alarmId = 0;     // edge alarm in the shared pool when no hardware alarm was spare
            void forwardHead();
            uint8_t transmit();

            enum class Job : uint8_t
            {
                None,
                Edge,
                Sync,
                Input,
                Queued
            };
            enum : uint8_t
            {
                kSyncIdle,
                kSyncQueued,
                kSyncRunning,
                kSyncDone
            };
            struct Input
            {
                uint8_t address;
                bool commandAfterRead;
            };
            enum : uint8_t
            {
                kQueueFree,
                kQueueWaiting,
                kQueueFirst,  // first segment on the bus
                kQueueSecond, // second segment waiting or on the bus
                kQueueOk,
                kQueueFailed
            };
            struct Queued
            {
                uint32_t tag;
                uint8_t address;
                uint8_t firstLen;
                uint8_t secondLen;
                uint8_t state;
                uint8_t first[8];   // display: control byte + page/column window
                uint8_t second[33]; // display: data-mode byte + 32 columns
            };

            spin_lock_t* _lock = nullptr; // engine state is shared by both cores and the IRQs
            bool _hwUp = false;           // TwoWire begun
            bool _engine = false;         // I2C IRQ handler installed and alarm running
            bool _engineProfile = false;  // block registers set for the engine, not for TwoWire's polling
            volatile bool _fallback = false; // a TwoWire polling transfer owns the block
            bool _txLong = false;         // write longer than _txHead: handed to TwoWire

            // transfer on the bus
            volatile Job _job = Job::None;
            const uint8_t* _xTx = nullptr;
            uint8_t* _xRx = nullptr;
            uint8_t _xTxLen = 0;
            uint8_t _xTotal = 0;  // FIFO entries: write bytes + read commands
            uint8_t _xPushed = 0;
            uint8_t _xRxLen = 0;
            uint8_t _xRxGot = 0;
            uint8_t _xPorts = 0;  // edge: port written; sync: ports merged into the write
            bool _xError = false;
            bool _xTimeout = false;
            uint32_t _xStartUs = 0;
            uint32_t _xLimitUs = 0;
            uint32_t _xProgress = 0; // IRQ events of this transfer
            uint32_t _wdProgress = 0;
            uint32_t _wdSinceUs = 0;
            bool _wdArmed = false;

            // LED edges
            volatile bool _edgeDue = false;
            uint8_t _edgePort = 0; // next port to check
            uint8_t _edgeBuf[2] = {};
        #ifdef OPENKNX_CONSOLE_DEV
            uint32_t _edgeAtUs = 0;     // planned time of the pending event
            bool _edgeLateOpen = false; // its first write counts for late/maxLateUs
        #endif

            // transfer of the waiting caller
            volatile uint8_t _syncState = kSyncIdle;
            uint8_t _syncAddress = 0;
            uint8_t _syncTxLen = 0;
            uint8_t _syncRxLen = 0;
            volatile uint8_t _syncResult = 0;
            bool _syncPoll = false; // caller cannot be interrupted: started without waiting for a gap
            uint32_t _syncSinceUs = 0;
            uint32_t _syncEstUs = 0;
            uint32_t _syncLimitUs = 0; // TwoWire timeout
            uint8_t _rxBuf[16] = {}; // RX FIFO depth: all read commands of a transfer fit in flight
            uint8_t _rxLen = 0;
            uint8_t _rxOff = 0;
            bool _rxOwn = false;     // last read came from the engine, not from TwoWire

            // input cache
            Input _inputs[OPENKNX_I2C_SCHEDULER_MAX_INPUTS] = {};
            volatile uint32_t _inputWord[OPENKNX_I2C_SCHEDULER_MAX_INPUTS] = {}; // bit 31 valid, bits 0..7 inputs
            uint32_t _inputUs[OPENKNX_I2C_SCHEDULER_MAX_INPUTS] = {};
            volatile uint8_t _inputCount = 0;
            uint8_t _inputNext = 0;
            uint8_t _inputStep = 0; // 0 read, 1 command byte after the read
            uint8_t _inputFailing = 0; // bit per input: last read failed, the value is kept like PCA95XX does
            volatile bool _inputDue = false;
            uint32_t _inputLastUs = 0;
            uint8_t _inputTx = 0;
            uint8_t _inputRx = 0;

            // queued writes: ring in caller order; send pointer between taken-back and free slots
            Queued _queue[OPENKNX_I2C_SCHEDULER_QUEUE] = {};
            uint8_t _queueHead = 0;  // next free slot
            uint8_t _queueSend = 0;  // slot on the bus or next for it
            uint8_t _queueTail = 0;  // oldest slot not taken back
            uint8_t _queueCount = 0; // slots in use
            bool _queueMid = false;  // first segment sent, second pending
            uint32_t _queueMidUs = 0;
            uint32_t _queueWaitUs = 0; // the slot at _queueSend waits for the bus since then
            bool _queueForced = false; // the pair on the bus started without a gap: its second segment follows at once

            bool interruptible() const;
            void service();
            void pump();
            void kick(bool afterEvent);
            bool fits(uint32_t now, uint32_t durationUs) const;
            void startTransfer(Job job, uint8_t address, const uint8_t* tx, uint8_t txLen, uint8_t* rx, uint8_t rxLen, uint32_t limitUs);
            bool startEdge(uint32_t now);
            void startSync(uint32_t now, bool forced);
            void startInput();
            void startQueued();
            void complete(bool ok);
            void watchdog(uint32_t now);
            void recover();
            void resetBus();
            void engineProfile();
            void pollingProfile();
            void pollInputs(uint32_t now);
            uint8_t runSync(uint8_t address, uint8_t txLen, uint8_t rxLen);
            bool enterFallback(uint32_t durationUs, bool driverTransfer);
            void exitFallback(bool ok);
            void startEngine();
            uint8_t fallbackWrite();
            size_t fallbackRead(uint8_t address, size_t len, bool writeFirst);
    #endif

    #ifdef ARDUINO_ARCH_ESP32
            inline uint32_t schedLock()
            {
                portENTER_CRITICAL(&_mux);
                return 0;
            }
            inline void schedUnlock(uint32_t) { portEXIT_CRITICAL(&_mux); }
            void grant(uint32_t durationUs, bool driverTransfer = true);
            void release();
            void writePorts(uint32_t now);
            bool writeRaw(uint8_t address, uint8_t reg, uint8_t value);
    #else
            inline uint32_t schedLock() { return _lock ? spin_lock_blocking(_lock) : save_and_disable_interrupts(); }
            inline void schedUnlock(uint32_t state)
            {
                if (_lock)
                    spin_unlock(_lock, state);
                else
                    restore_interrupts(state);
            }
    #endif
            uint32_t transferUs(size_t writeBytes, size_t readBytes) const;
            void updateTiming();
            uint8_t mergeOutputWrite(uint8_t address, uint8_t* data, size_t len);
            void commitOutputWrite(uint8_t ports, bool ok);
            void beginPeriod(uint32_t now, bool lightDimmed);
            void advanceEvent(uint32_t now, bool lightDimmed = true);
            bool dimmedLeds() const;
            /// @brief The LED gets an off-edge in the period (dimmed, not too close to full).
            inline bool hasOffEdge(uint8_t brightness) const
            {
                return brightness != 0 && brightness != 255 && (uint32_t)brightness * periodUs() / 255 <= periodUs() - _edgeUs;
            }
            bool start();
        };
        static_assert(ScheduledWire::periodUs() <= 0xFFFF, "OPENKNX_I2C_SCHEDULER_PWM_HZ too low for 16-bit edge offsets");
        static_assert(OPENKNX_I2C_SCHEDULER_MAX_PORTS <= 8, "port bitmask is 8 bit");
    #ifdef ARDUINO_ARCH_RP2040
        static_assert(OPENKNX_I2C_SCHEDULER_MAX_INPUTS <= 8, "input bitmask is 8 bit");
    #endif
    } // namespace I2C
} // namespace OpenKNX

extern OpenKNX::I2C::ScheduledWire openknxScheduledWire;

#endif // OPENKNX_I2C_SCHEDULER
