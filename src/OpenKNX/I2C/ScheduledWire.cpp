/**
 * @file        ScheduledWire.cpp
 * @brief       Time-slotted owner of one I2C bus (see ScheduledWire.h)
 * @version     0.0.1
 * @date        2026-10-23
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#include "OpenKNX/I2C/ScheduledWire.h"

#ifdef OPENKNX_I2C_SCHEDULER
    #include "OpenKNX/Facade.h"
    #ifdef ARDUINO_ARCH_ESP32
        #include "esp32-hal-i2c.h"
        #include "esp_timer.h"
    #else
        #include "hardware/clocks.h"
        #include "pico/time.h"
    #endif

OpenKNX::I2C::ScheduledWire openknxScheduledWire;

    #ifdef OPENKNX_CONSOLE_DEV
        #define SCHED_STAT(...) __VA_ARGS__ // console counters (leds pwm status)
    #else
        #define SCHED_STAT(...)
    #endif

namespace
{
    constexpr uint32_t kPeriodUs = OpenKNX::I2C::ScheduledWire::periodUs();

    inline uint32_t nowUs()
    {
    #ifdef ARDUINO_ARCH_ESP32
        return (uint32_t)esp_timer_get_time();
    #else
        return time_us_32();
    #endif
    }

    #ifdef ARDUINO_ARCH_ESP32
    TaskHandle_t s_busTask = nullptr;
    esp_timer_handle_t s_eventTimer = nullptr;

    void armEventTimer()
    {
        int32_t delta = (int32_t)(openknxScheduledWire.nextEventUs() - nowUs());
        if (delta < 1)
            delta = 1;
        esp_timer_stop(s_eventTimer); // ESP_ERR_INVALID_STATE when not running is fine
        esp_timer_start_once(s_eventTimer, (uint64_t)delta);
    }

    void busTask(void*)
    {
        for (;;)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            if (openknxScheduledWire.runEvents())
                armEventTimer();
            // bus busy: release() notifies again when the transfer ends
        }
    }
    #endif
} // namespace

namespace OpenKNX
{
    namespace I2C
    {
        ScheduledWire::ScheduledWire()
    #ifdef ARDUINO_ARCH_ESP32
            : TwoWire(OPENKNX_I2C_SCHEDULER_BUS)
    #else
            : TwoWire(i2c_get_instance(OPENKNX_I2C_SCHEDULER_BUS), OPENKNX_GPIO_SDA, OPENKNX_GPIO_SCL),
              _inst(i2c_get_instance(OPENKNX_I2C_SCHEDULER_BUS))
    #endif
        {
            updateTiming();
        }

        // ---------------------------------------------------------------- timing / bus ownership

        uint32_t SCHED_RAM(ScheduledWire::transferUs)(size_t writeBytes, size_t readBytes) const
        {
            uint32_t bits = 2; // START + STOP
            if (writeBytes || !readBytes)
                bits += (1 + writeBytes) * 9; // address + data; a 0-byte write is an address probe
            if (readBytes)
                bits += 1 + (1 + readBytes) * 9; // repeated START + address + data
    #ifdef ARDUINO_ARCH_ESP32
            const uint32_t kHz = _clkHz >= 1000 ? _clkHz / 1000 : 1;
            return bits * 1000UL / kHz + OPENKNX_I2C_SCHEDULER_OVERHEAD_US;
    #else
            return (bits * _bitNs + 999) / 1000 + OPENKNX_I2C_SCHEDULER_OVERHEAD_US;
    #endif
        }

        void ScheduledWire::updateTiming()
        {
    #ifdef ARDUINO_ARCH_RP2040
            // SCL period as i2c_set_baudrate() programs it (DW_apb_i2c: low LCNT + 1, high HCNT + SPKLEN + 7 cycles),
            // plus the rise time, which the controller waits for before it counts the high time
            const uint32_t freq = clock_get_hz(clk_sys);
            const uint32_t hz = _clkHz >= 1000 ? _clkHz : 1000;
            if (freq >= hz * 20) // i2c_set_baudrate() needs lcnt and hcnt >= 8
            {
                const uint32_t period = (freq + hz / 2) / hz;
                const uint32_t lcnt = period * 3 / 5;
                const uint32_t hcnt = period - lcnt;
                const uint32_t spklen = lcnt < 16 ? 1 : lcnt / 16;
                _bitNs = (uint32_t)((uint64_t)(lcnt + 1 + hcnt + spklen + 7) * 1000000000ULL / freq) + OPENKNX_I2C_SCHEDULER_RISE_NS;
            }
            else
                _bitNs = 1000000000UL / hz + OPENKNX_I2C_SCHEDULER_RISE_NS; // clock not known yet (static init)
    #endif
            _edgeUs = transferUs(2, 0) + kMarginUs;
        }

    #ifdef ARDUINO_ARCH_ESP32
        // Wait until the transfer ends before the next edge, then own the bus. A transfer longer than
        // any gap gets the bus after one period; the next edge then waits for it.
        void ScheduledWire::grant(uint32_t durationUs, bool driverTransfer)
        {
            const uint32_t start = nowUs();
            for (;;)
            {
                const uint32_t state = schedLock();
                const uint32_t now = nowUs();
                const bool fits = !_running || !_nextEventBlocks || (int32_t)(_nextEventUs - now) > (int32_t)(durationUs + kMarginUs);
                const bool forced = now - start > kPeriodUs;
                if (!_busy && (fits || forced))
                {
                    _busy = true;
                    schedUnlock(state);
                    SCHED_STAT(if (driverTransfer && !_inStress) {
                        _stats.transfers++;
                        if (now - start > _stats.maxWaitUs)
                            _stats.maxWaitUs = now - start;
                        if (!fits)
                            _stats.forced++;
                        _stats.waitUs += now - start;
                        _grantUs = now;
                        _grantCounted = true;
                    });
                    (void)driverTransfer;
                    return;
                }
                schedUnlock(state);
            }
        }

        void ScheduledWire::release()
        {
            SCHED_STAT(if (_grantCounted) {
                _stats.transferUs += nowUs() - _grantUs;
                _grantCounted = false;
            });
            const uint32_t state = schedLock();
            _busy = false;
            const bool eventWaiting = _eventWaiting;
            _eventWaiting = false;
            schedUnlock(state);
            if (eventWaiting && s_busTask)
                xTaskNotifyGive(s_busTask);
        }
    #endif

        // ---------------------------------------------------------------- output-register merge

        // A driver write to an LED output register keeps its non-LED bits; the LED bits get the PWM level.
        // Runs while the bus is owned, so the PWM state cannot change before the transfer.
        uint8_t SCHED_RAM(ScheduledWire::mergeOutputWrite)(uint8_t address, uint8_t* data, size_t len)
        {
            uint8_t ports = 0;
            if (len < 2)
                return 0;
            const uint8_t portCount = _portCount;
            for (uint8_t p = 0; p < portCount; p++)
            {
                Port& port = _ports[p];
                if (port.address != address)
                    continue;
                for (size_t i = 1; i < len; i++)
                {
                    // auto-increment toggles within the register pair (TCA9555 0x02 <-> 0x03)
                    const uint8_t reg = port.autoIncrement ? (uint8_t)(data[0] ^ ((i - 1) & 1)) : data[0];
                    if (reg != port.reg)
                        continue;
                    port.other = data[i];
                    data[i] = (data[i] & ~port.ledMask) | ((port.onMask ^ port.activeLowMask) & port.ledMask);
                    port.pending = data[i];
                    ports |= 1 << p;
                }
            }
            return ports;
        }

        void SCHED_RAM(ScheduledWire::commitOutputWrite)(uint8_t ports, bool ok)
        {
            for (uint8_t p = 0; ports; p++, ports >>= 1)
            {
                if (!(ports & 1))
                    continue;
                _ports[p].written = _ports[p].pending;
                _ports[p].writtenValid = ok;
            }
        }

        // ---------------------------------------------------------------- PWM engine

        // Period start: LEDs on; otherwise the off-edges that are due.
        void SCHED_RAM(ScheduledWire::advanceEvent)(uint32_t now, bool lightDimmed)
        {
            if (_offIndex >= _offCount)
                beginPeriod(now, lightDimmed);
            else
                while (_offIndex < _offCount && (int32_t)(now - (_periodStartUs + _off[_offIndex].atUs)) >= -(int32_t)kSlackUs)
                {
                    _ports[_off[_offIndex].port].onMask &= ~_off[_offIndex].mask;
                    _offIndex++;
                }
        }

    #ifdef ARDUINO_ARCH_ESP32
        bool ScheduledWire::runEvents()
        {
            uint32_t state = schedLock();
            if (_busy)
            {
                _eventWaiting = true;
                SCHED_STAT(_stats.busyRetries++); // under the lock, so resetStats() cannot interleave
                schedUnlock(state);
                return false;
            }
            _busy = true;
            schedUnlock(state);

            const uint32_t now = nowUs();
            const int32_t lateUs = (int32_t)(now - _nextEventUs);
            if (lateUs >= -(int32_t)kSlackUs) // an early wake-up only re-arms
            {
                SCHED_STAT(_stats.events++; if (_startBlocks) { // a non-blocking period start may wait for a transfer by design
                    if (lateUs > (int32_t)kLateUs)
                        _stats.late++;
                    if (lateUs > 0 && (uint32_t)lateUs > _stats.maxLateUs)
                        _stats.maxLateUs = lateUs;
                });

                advanceEvent(now);
                writePorts(now);
            }

            state = schedLock();
            _nextEventUs = _offIndex < _offCount ? _periodStartUs + _off[_offIndex].atUs : _periodStartUs + kPeriodUs;
            _startBlocks = _offCount > 0 || dimmedLeds(); // static LEDs: the next period start does not hold other transfers back
            _nextEventBlocks = _startBlocks;
            _busy = false;
            schedUnlock(state);
            return true;
        }
    #endif

        // A dimmed LED read after the last event decision still needs a free next period start.
        bool SCHED_RAM(ScheduledWire::dimmedLeds)() const
        {
            const uint8_t ledCount = _ledCount;
            for (uint8_t i = 0; i < ledCount; i++)
                if (hasOffEdge(_leds[i].brightness))
                    return true;
            return false;
        }

        // Switch all lit LEDs on and plan their off-edges for this period. Without lightDimmed (a transfer runs
        // over this start) a newly dimmed LED keeps its last state for one period, so no pulse is stretched.
        void SCHED_RAM(ScheduledWire::beginPeriod)(uint32_t now, bool lightDimmed)
        {
            uint8_t lastOn[OPENKNX_I2C_SCHEDULER_MAX_PORTS];
            uint32_t start = _periodStartUs + kPeriodUs;
            if (!_periodValid || (int32_t)(now - start) > (int32_t)kPeriodUs)
                start = now; // first period, or resync after a stall (flash write, long transfer)
            _periodStartUs = start;
            _periodValid = true;

            const uint8_t portCount = _portCount;
            for (uint8_t p = 0; p < portCount; p++)
            {
                lastOn[p] = _ports[p].onMask;
                _ports[p].onMask = 0;
            }

            _offCount = 0;
            _offIndex = 0;
            const uint8_t ledCount = _ledCount;
            for (uint8_t i = 0; i < ledCount; i++)
            {
                const Led& led = _leds[i];
                const uint8_t brightness = led.brightness;
                if (brightness == 0)
                    continue;
                uint32_t onUs = (uint32_t)brightness * kPeriodUs / 255;
                const bool full = !hasOffEdge(brightness); // full, or an off-edge too close to the next on-edge: stays on
                if (!full && !lightDimmed)
                {
                    _ports[led.port].onMask |= lastOn[led.port] & led.mask; // fading down from full: stays on, from off: stays off
                    continue;
                }
                _ports[led.port].onMask |= led.mask; // any brightness > 0 lights, at least for the shortest pulse
                if (full)
                    continue;
                if (onUs < _edgeUs)
                    onUs = _edgeUs;
                uint8_t j = _offCount++;
                while (j > 0 && (uint32_t)_off[j - 1].atUs > onUs)
                {
                    _off[j] = _off[j - 1];
                    j--;
                }
                _off[j] = {(uint16_t)onUs, led.port, led.mask};
            }
            // off-edges closer than one write run together
            for (uint8_t j = 1; j < _offCount; j++)
                if ((uint32_t)(_off[j].atUs - _off[j - 1].atUs) < _edgeUs)
                    _off[j].atUs = _off[j - 1].atUs;
        }

    #ifdef ARDUINO_ARCH_ESP32
        void ScheduledWire::writePorts(uint32_t now)
        {
            const uint8_t portCount = _portCount;
            for (uint8_t p = 0; p < portCount; p++)
            {
                Port& port = _ports[p];
                if (port.holding)
                {
                    if ((int32_t)(now - port.holdUntilUs) < 0)
                        continue; // bus errors: no writes to this port for kHoldUs, a stuck bus must not eat the CPU
                    port.holding = false;
                }
                const uint8_t value = (port.other & ~port.ledMask) | ((port.onMask ^ port.activeLowMask) & port.ledMask);
                if (port.writtenValid && value == port.written)
                    continue;
                SCHED_STAT(const uint32_t writeStart = nowUs());
                port.writtenValid = writeRaw(port.address, port.reg, value); // failed: retried on the next event
                SCHED_STAT(_stats.edgeWriteUs += nowUs() - writeStart);
                port.written = value;
                if (port.writtenValid)
                {
                    port.failures = 0;
                    continue;
                }
                SCHED_STAT(_stats.edgeErrors++);
                if (++port.failures >= kHoldFailures)
                {
                    port.failures = 0;
                    port.holdUntilUs = now + kHoldUs;
                    port.holding = true;
                }
            }
        }

        bool ScheduledWire::writeRaw(uint8_t address, uint8_t reg, uint8_t value)
        {
            const uint8_t buf[2] = {reg, value};
            return i2cWrite(num, address, buf, 2, 2) == ESP_OK;
        }

        // Starts the PWM engine; false if its task/timer could not be created (LEDs then stay on the pending-flag path).
        bool ScheduledWire::start()
        {
            if (_running)
                return true;
            _periodValid = false;
            _offCount = 0;
            _offIndex = 0;
            _nextEventUs = nowUs() + 1000;
            // above loop task and lwIP, below the esp_timer service task that wakes it
            if (xTaskCreatePinnedToCore(busTask, "OpenKnxI2C", 3072, nullptr, configMAX_PRIORITIES - 4, &s_busTask, tskNO_AFFINITY) != pdPASS)
                return false;
            const esp_timer_create_args_t timerArgs = {
                .callback = [](void*) { xTaskNotifyGive(s_busTask); },
                .arg = nullptr,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "OpenKnxI2C",
                .skip_unhandled_events = true};
            if (esp_timer_create(&timerArgs, &s_eventTimer) != ESP_OK)
            {
                vTaskDelete(s_busTask);
                s_busTask = nullptr;
                return false;
            }
            _running = true;
            armEventTimer();
            return true;
        }
    #endif

        int8_t ScheduledWire::addLed(uint8_t address, uint8_t reg, uint8_t bit, bool activeHigh, bool autoIncrement)
        {
            if (_ledCount >= OPENKNX_I2C_SCHEDULER_MAX_LEDS || bit > 7)
                return -1;

            uint8_t p = 0;
            while (p < _portCount && !(_ports[p].address == address && _ports[p].reg == reg))
                p++;
            uint8_t seed = 0;
            if (p == _portCount)
            {
                if (p >= OPENKNX_I2C_SCHEDULER_MAX_PORTS)
                    return -1;
                // seed the non-LED bits from the chip
                beginTransmission(address);
                write(reg);
                if (endTransmission(false) != 0 || requestFrom(address, (size_t)1) != 1)
                    return -1;
                seed = (uint8_t)read();
            }
            if (!start())
                return -1;

            // publish while owning the bus: the engine never sees a half-set port or LED
    #ifdef ARDUINO_ARCH_RP2040
            const uint32_t state = schedLock();
    #else
            grant(0, false);
    #endif
            if (p == _portCount)
            {
                Port& port = _ports[p];
                port = {};
                port.address = address;
                port.reg = reg;
                port.autoIncrement = autoIncrement;
                port.other = seed;
                port.written = seed;
                port.writtenValid = true;
                _portCount = p + 1;
            }
            const uint8_t mask = 1 << bit;
            Port& port = _ports[p];
            port.ledMask |= mask;
            if (activeHigh)
                port.activeLowMask &= ~mask;
            else
                port.activeLowMask |= mask;
            const uint8_t index = _ledCount;
            _leds[index].port = p;
            _leds[index].mask = mask;
            _leds[index].brightness = 0;
            _ledCount = index + 1;
    #ifdef ARDUINO_ARCH_RP2040
            schedUnlock(state);
    #else
            release();
    #endif
            return (int8_t)index;
        }

    #ifdef OPENKNX_CONSOLE_DEV
        ScheduledWire::Stats ScheduledWire::stats()
        {
        #ifdef ARDUINO_ARCH_RP2040
            const uint32_t state = schedLock(); // IRQ-side counters are written under the lock
            const Stats copy = _stats;
            schedUnlock(state);
        #else
            grant(0, false); // the engine cannot run, so it writes no counter meanwhile ...
            const uint32_t state = schedLock(); // ... and busyRetries is written under the lock
            const Stats copy = _stats;
            schedUnlock(state);
            release();
        #endif
            return copy;
        }

        void ScheduledWire::resetStats()
        {
        #ifdef ARDUINO_ARCH_RP2040
            const uint32_t state = schedLock(); // IRQ-side counters are written under the lock
            _stats = {};
            _stats.sinceMs = millis();
            schedUnlock(state);
        #else
            grant(0, false); // the engine cannot run, so only busyRetries can change ...
            const uint32_t state = schedLock(); // ... and that one is written under the lock
            _stats = {};
            _stats.sinceMs = millis();
            schedUnlock(state);
            release();
        #endif
        }

        // ---------------------------------------------------------------- bus stress

        void ScheduledWire::loop()
        {
            if (!_stressBus || !_portCount)
                return;
            const uint8_t address = _ports[0].address;
            _inStress = true;
            for (uint8_t i = 0; i < kStressReads; i++)
            {
                beginTransmission(address);
                write(0x00); // register 0 = input port on PCA9554/9557, TCA6408/9555
                endTransmission(false);
                _stats.stressReads++;
                if (requestFrom(address, (size_t)1) == 1)
                    read();
                else
                    _stats.stressErrors++;
            }
            // pointer off the input register (PCA9554 errata), a command byte without data writes nothing
            beginTransmission(address);
            write(0x01);
            endTransmission();
            _inStress = false;
        }
    #endif

        // ---------------------------------------------------------------- TwoWire

    #ifdef ARDUINO_ARCH_ESP32
        #ifdef OPENKNX_CONSOLE_DEV
        uint32_t ScheduledWire::busTaskStackFree() const
        {
            return s_busTask ? (uint32_t)uxTaskGetStackHighWaterMark(s_busTask) : 0;
        }
        #endif

        // Same as TwoWire::endTransmission(), the transfer waits for a gap; TwoWire's lock is kept per call.
        uint8_t ScheduledWire::endTransmission(bool stopBit)
        {
            if (txBuffer == NULL)
                return 4;
            if (!stopBit)
            {
                nonStop = true; // sent by the following requestFrom(), like TwoWire
                return 0;
            }
            grant(transferUs(txLength, 0));
            const uint8_t ports = mergeOutputWrite((uint8_t)txAddress, txBuffer, txLength);
            const esp_err_t err = i2cWrite(num, txAddress, txBuffer, txLength, _timeOutMillis);
            commitOutputWrite(ports, err == ESP_OK);
            release();
            SCHED_STAT(if (err != ESP_OK && !_inStress) _stats.transferErrors++);
        #if !CONFIG_DISABLE_HAL_LOCKS
            currentTaskHandle = NULL;
            xSemaphoreGive(lock);
        #endif
            switch (err)
            {
                case ESP_OK:
                    return 0;
                case ESP_FAIL:
                case ESP_ERR_NOT_FOUND:
                    return 2;
                case ESP_ERR_TIMEOUT:
                    return 5;
                default:
                    return 4;
            }
        }

        size_t ScheduledWire::requestFrom(uint8_t address, size_t len, bool stopBit)
        {
            (void)stopBit; // TwoWire on ESP32 ends every read with STOP
            if (rxBuffer == NULL || txBuffer == NULL)
                return 0;
        #if !CONFIG_DISABLE_HAL_LOCKS
            TaskHandle_t task = xTaskGetCurrentTaskHandle();
            if (currentTaskHandle != task)
            {
                if (lock == NULL || xSemaphoreTake(lock, portMAX_DELAY) != pdTRUE)
                    return 0;
                currentTaskHandle = task;
            }
        #endif
            const bool writeFirst = nonStop;
            nonStop = false;
            rxIndex = 0;
            rxLength = 0;
            if (!writeFirst || address == txAddress) // a repeated START to another address is rejected like in TwoWire
            {
                if (len > bufferSize)
                    len = bufferSize;
                grant(transferUs(writeFirst ? txLength : 0, len));
                uint8_t ports = 0;
                if (writeFirst)
                {
                    ports = mergeOutputWrite((uint8_t)txAddress, txBuffer, txLength);
                    i2cWriteReadNonStop(num, address, txBuffer, txLength, rxBuffer, len, _timeOutMillis, &rxLength);
                }
                else
                    i2cRead(num, address, rxBuffer, len, _timeOutMillis, &rxLength);
                commitOutputWrite(ports, rxLength == len);
                release();
                SCHED_STAT(if (rxLength != len && !_inStress) _stats.transferErrors++);
            }
        #if !CONFIG_DISABLE_HAL_LOCKS
            currentTaskHandle = NULL;
            xSemaphoreGive(lock);
        #endif
            return rxLength;
        }

        bool ScheduledWire::setClock(uint32_t freq)
        {
            if (freq == _clkHz)
                return true; // drivers set the same clock before every transfer
            grant(OPENKNX_I2C_SCHEDULER_OVERHEAD_US, false);
            const bool ok = TwoWire::setClock(freq);
            release();
            if (ok)
            {
                _clkHz = freq;
                updateTiming();
            }
            return ok;
        }
    #endif
    } // namespace I2C
} // namespace OpenKNX

#endif // OPENKNX_I2C_SCHEDULER
