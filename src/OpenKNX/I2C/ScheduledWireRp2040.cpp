/**
 * @file        ScheduledWireRp2040.cpp
 * @brief       IRQ transfer engine of ScheduledWire on RP2040/RP2350
 * @version     0.0.1
 * @date        2026-10-23
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Transfers of up to 33 write and 16 read bytes run from the I2C IRQ through the hardware FIFOs; LED edges
 * are 2-byte transfers started from the alarm IRQ. Bus order: LED edges, the second segment of a queued
 * write, the waiting caller, input-cache reads, queued writes; a transfer starts only when it ends before
 * the next LED edge. Address probes, longer transfers and setClock() run on TwoWire's polling path while
 * the engine is paused (fallback).
 * The alarm and the I2C IRQ run on core 0. Callers may use either core, one caller at a time.
 * Watched expander inputs may be up to 10 ms old (50 ms while reads starve).
 */
#include "OpenKNX/I2C/ScheduledWire.h"

#if defined(OPENKNX_I2C_SCHEDULER) && defined(ARDUINO_ARCH_RP2040)
    #include "OpenKNX/Facade.h"
    #include "hardware/gpio.h"
    #include "hardware/irq.h"
    #include "hardware/timer.h"
    #include "pico/platform.h"

    #ifdef OPENKNX_CONSOLE_DEV
        #define SCHED_STAT(...) __VA_ARGS__ // console counters (leds pwm status)
    #else
        #define SCHED_STAT(...)
    #endif

namespace
{
    constexpr uint32_t kPeriodUs = OpenKNX::I2C::ScheduledWire::periodUs();
    constexpr uint8_t kFifoDepth = 16;
    constexpr uint8_t kRefillLevel = 8;         // TX_EMPTY at or below this TX FIFO level
    constexpr uint32_t kRecoverMarginUs = 1000; // added to the limit of a transfer; also the no-progress window
    constexpr uint32_t kMaxTimeoutMs = 60000;   // cap of the TwoWire timeout used as limit of a caller's transfer
    constexpr uint32_t kInputPeriodUs = 10000;  // input cache: read interval ...
    constexpr uint32_t kInputMaxAgeUs = 50000;  // ... values older than this are dropped
    constexpr uint32_t kInputValid = 1UL << 31;
    constexpr uint8_t kInputRegister = 0x00;
    constexpr uint8_t kCommandAfterRead = 0x01; // PCA9554 errata: command byte off the input register
    constexpr uint32_t kEndIrqs = I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
    // Own edge alarm above the default IRQ priority: the OpenKNX tick in the shared alarm pool takes up to ~90 us
    // and would delay the edges. The engine handlers run with IRQs off anyway, so the longest block for others is unchanged.
    constexpr uint8_t kEdgeIrqPriority = PICO_DEFAULT_IRQ_PRIORITY - 0x40;
    constexpr uint32_t kMinArmUs = 20; // shortest alarm lead, longer than setting it
    int s_edgeAlarm = -1;

    inline uint32_t nowUs() { return time_us_32(); }

    void __not_in_flash_func(i2cIrq)() { openknxScheduledWire.onIrq(); }

    int64_t eventAlarm(alarm_id_t, void*)
    {
        openknxScheduledWire.runEvents();
        const int32_t delta = (int32_t)(openknxScheduledWire.nextEventUs() - nowUs());
        return delta < 1 ? 1 : delta;
    }

    // Hardware alarm: re-armed after the scheduler lock is released (the timer spinlock is never taken inside it)
    void __not_in_flash_func(edgeAlarm)(uint alarmNum)
    {
        for (;;)
        {
            openknxScheduledWire.runEvents();
            const uint64_t now = time_us_64();
            int32_t delta = (int32_t)(openknxScheduledWire.nextEventUs() - (uint32_t)now);
            if (delta < (int32_t)kMinArmUs)
                delta = kMinArmUs;
            if (!hardware_alarm_set_target(alarmNum, from_us_since_boot(now + delta)))
                return; // armed; missed only after more than kMinArmUs in here: run the due events again
        }
    }

    // A free hardware alarm, leaving at least one free for code that claims one later.
    int claimEdgeAlarm()
    {
        uint8_t unclaimed = 0;
        for (uint i = 0; i < NUM_ALARMS; i++)
            if (!hardware_alarm_is_claimed(i))
                unclaimed++;
        return unclaimed >= 2 ? hardware_alarm_claim_unused(false) : -1;
    }
} // namespace

namespace OpenKNX
{
    namespace I2C
    {
        // ---------------------------------------------------------------- engine setup

        void ScheduledWire::begin()
        {
            if (_hwUp)
                return; // TwoWire::begin() ignores it as well
            if (!_lock && get_core_num() == 0)
            {
                const int lock = spin_lock_claim_unused(false);
                if (lock >= 0)
                    _lock = spin_lock_instance(lock);
            }
            const uint32_t state = schedLock();
            TwoWire::begin();
            _inst->hw->intr_mask = 0; // reset value 0x8FF; the engine unmasks per transfer
            _hwUp = true;
            _engineProfile = false;
            schedUnlock(state);
            updateTiming(); // clk_sys is known now
            if (!_engine && _lock)
                startEngine();
        }

        // I2C IRQ and edge alarm on core 0; without them every transfer takes TwoWire's polling path.
        void ScheduledWire::startEngine()
        {
            alarm_pool_t* pool = openknx.timerInterrupt.alarmPool();
            s_edgeAlarm = claimEdgeAlarm();
            if (s_edgeAlarm < 0 && !pool)
                return;
            const uint irq = I2C0_IRQ + OPENKNX_I2C_SCHEDULER_BUS;
            irq_set_exclusive_handler(irq, i2cIrq);
            irq_set_enabled(irq, true);
            _periodValid = false;
            _offCount = 0;
            _offIndex = 0;
            _nextEventUs = nowUs() + 1000;
            _running = true;
            if (s_edgeAlarm >= 0)
            {
                hardware_alarm_set_callback(s_edgeAlarm, edgeAlarm); // handler on this core (core 0)
                irq_set_priority(hardware_alarm_get_irq_num(s_edgeAlarm), kEdgeIrqPriority);
                hardware_alarm_set_target(s_edgeAlarm, make_timeout_time_us(1000));
            }
            else
            {
                _alarmId = alarm_pool_add_alarm_in_us(pool, 1000, eventAlarm, nullptr, true); // no spare alarm: shared pool
                if (_alarmId <= 0)
                {
                    _running = false;
                    irq_set_enabled(irq, false);
                    irq_remove_handler(irq, i2cIrq);
                    return;
                }
            }
            _engine = true;
        }

        // The engine runs from begin(); LEDs need it.
        bool ScheduledWire::start()
        {
            return _engine;
        }

        void ScheduledWire::end()
        {
            if (!_hwUp)
                return;
            const bool own = !_inTransfer; // TwoWire's timeout recovery calls end()/begin() inside a transfer
            if (own && !enterFallback(0, false))
                return;
            const uint32_t state = schedLock();
            _hwUp = false; // the engine leaves the block alone until begin()
            _engineProfile = false;
            TwoWire::end();
            schedUnlock(state);
            if (!own)
                return;
            _txOpen = false; // TwoWire drops an open transmission as well
            _txLong = false;
            _deferredWrite = false;
            _txLen = 0;
            exitFallback(true);
        }

        // Block idle, IC_ENABLE written to 0 here; the next transfer enables it.
        void ScheduledWire::engineProfile()
        {
            i2c_hw_t* hw = _inst->hw;
            hw->intr_mask = 0;
            hw->enable = 0;
            hw_clear_bits(&hw->con, I2C_IC_CON_TX_EMPTY_CTRL_BITS); // TX_EMPTY by FIFO level, not per sent byte
            hw->tx_tl = kRefillLevel;
            hw->rx_tl = 0;
            _engineProfile = true;
        }

        // Registers as i2c_init() leaves them, for TwoWire's polling transfers.
        void ScheduledWire::pollingProfile()
        {
            i2c_hw_t* hw = _inst->hw;
            hw->intr_mask = 0;
            hw->enable = 0;
            hw_set_bits(&hw->con, I2C_IC_CON_TX_EMPTY_CTRL_BITS);
            hw->tx_tl = 0;
            hw->rx_tl = 0;
            hw->enable = 1;
            _inst->restart_on_next = false;
            _engineProfile = false;
        }

        // ---------------------------------------------------------------- IRQ side (lock held)

        void __not_in_flash_func(ScheduledWire::onIrq)()
        {
            const uint32_t state = schedLock();
            SCHED_STAT(const uint32_t start = nowUs());
            service();
            SCHED_STAT(_stats.edgeWriteUs += nowUs() - start);
            schedUnlock(state);
        }

        void __not_in_flash_func(ScheduledWire::service)()
        {
            if (!_hwUp)
                return; // block in reset
            i2c_hw_t* hw = _inst->hw;
            if (_job == Job::None)
            {
                hw->intr_mask = 0; // nothing of ours runs; TwoWire's polling reads the raw status itself
                return;
            }
            const uint32_t stat = hw->intr_stat;
            if (stat & I2C_IC_INTR_MASK_M_TX_ABRT_BITS)
            {
                _xError = true;
                const uint32_t source = hw->tx_abrt_source; // cleared by clr_tx_abrt
                (void)hw->clr_tx_abrt; // the controller sends STOP; STOP_DET ends the transfer
                hw->intr_mask = kEndIrqs;
                _xProgress++;
                if ((source & I2C_IC_TX_ABRT_SOURCE_ARB_LOST_BITS) && !(hw->status & I2C_IC_STATUS_ACTIVITY_BITS))
                {
                    // arbitration lost (single-master bus: a glitch or a slave out of step): no STOP of ours comes,
                    // a later foreign STOP would end the next transfer; the reset clears the status and the bus
                    resetBus();
                    complete(false);
                    return;
                }
            }
            else if (stat & I2C_IC_INTR_MASK_M_TX_EMPTY_BITS)
            {
                pump();
                _xProgress++;
            }
            if (stat & I2C_IC_INTR_MASK_M_STOP_DET_BITS)
            {
                (void)hw->clr_stop_det;
                while (hw->rxflr && _xRxGot < _xRxLen)
                    _xRx[_xRxGot++] = (uint8_t)hw->data_cmd;
                complete(!_xError && _xPushed == _xTotal && _xRxGot == _xRxLen);
            }
        }

        // Fill the TX FIFO with write bytes and read commands; TX_EMPTY stays unmasked while entries are left.
        void __not_in_flash_func(ScheduledWire::pump)()
        {
            i2c_hw_t* hw = _inst->hw;
            uint32_t room = kFifoDepth - hw->txflr;
            while (room && _xPushed < _xTotal)
            {
                room--;
                const uint8_t i = _xPushed++;
                uint32_t cmd;
                if (i < _xTxLen)
                    cmd = _xTx[i];
                else if (i == _xTxLen && _xTxLen)
                    cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS;
                else
                    cmd = I2C_IC_DATA_CMD_CMD_BITS;
                if (_xPushed == _xTotal)
                    cmd |= I2C_IC_DATA_CMD_STOP_BITS;
                hw->data_cmd = cmd;
            }
            hw->intr_mask = _xPushed < _xTotal ? (kEndIrqs | I2C_IC_INTR_MASK_M_TX_EMPTY_BITS) : kEndIrqs;
        }

        bool SCHED_RAM(ScheduledWire::fits)(uint32_t now, uint32_t durationUs) const
        {
            return !_engine || !_nextEventBlocks || (int32_t)(_nextEventUs - now) > (int32_t)(durationUs + kMarginUs);
        }

        // Start the next transfer if the bus is free: LED edges, then the waiting caller, then input reads.
        void SCHED_RAM(ScheduledWire::kick)(bool afterEvent)
        {
            if (_job != Job::None || _fallback || !_engine || !_hwUp)
                return;
            const uint32_t now = nowUs();
            if (_edgeDue)
            {
                if (startEdge(now))
                    return;
                _edgeDue = false;
            }
            if (_queueMid) // the first segment set an address window: nothing else before the second
            {
                const Queued& q = _queue[_queueSend];
                if (_queueForced || fits(now, transferUs(q.secondLen, 0)) || (_syncPoll && _syncState == kSyncQueued) ||
                    (afterEvent && now - _queueMidUs > kPeriodUs))
                    startQueued();
                return;
            }
            if (_syncState == kSyncQueued)
            {
                const bool fit = fits(now, _syncEstUs);
                // longer than any gap: right after an event, once it has waited one period
                if (fit || _syncPoll || (afterEvent && now - _syncSinceUs > kPeriodUs))
                {
                    startSync(now, !fit);
                    return;
                }
            }
            if (_inputDue && fits(now, transferUs(1, _inputStep ? 0 : 1)))
            {
                startInput();
                return;
            }
            if (_queueCount && _queue[_queueSend].state == kQueueWaiting)
            {
                const Queued& q = _queue[_queueSend];
                const bool fit = fits(now, transferUs(q.firstLen, 0) + transferUs(q.secondLen, 0)); // both segments in one gap
                if (fit || (afterEvent && now - _queueWaitUs > kPeriodUs))
                {
                    _queueForced = !fit;
                    startQueued();
                }
            }
        }

        void SCHED_RAM(ScheduledWire::startTransfer)(Job job, uint8_t address, const uint8_t* tx, uint8_t txLen, uint8_t* rx, uint8_t rxLen, uint32_t limitUs)
        {
            i2c_hw_t* hw = _inst->hw;
            if (!_engineProfile)
                engineProfile();
            hw->enable = 0;
            hw->tar = address;
            (void)hw->clr_intr; // stale STOP_DET/TX_ABRT; also releases a TX FIFO held after an abort
            hw->enable = 1;
            _job = job;
            _xTx = tx;
            _xTxLen = txLen;
            _xRx = rx;
            _xRxLen = rxLen;
            _xTotal = txLen + rxLen;
            _xPushed = 0;
            _xRxGot = 0;
            _xError = false;
            _xTimeout = false;
            _xStartUs = nowUs();
            _xLimitUs = limitUs;
            _xProgress = 0;
            _wdArmed = false;
            pump();
        }

        bool SCHED_RAM(ScheduledWire::startEdge)(uint32_t now)
        {
            const uint8_t portCount = _portCount;
            while (_edgePort < portCount)
            {
                const uint8_t p = _edgePort++;
                Port& port = _ports[p];
                if (port.holding)
                {
                    if ((int32_t)(now - port.holdUntilUs) < 0)
                        continue; // bus errors: no writes to this port for kHoldUs
                    port.holding = false;
                }
                const uint8_t value = (port.other & ~port.ledMask) | ((port.onMask ^ port.activeLowMask) & port.ledMask);
                if (port.writtenValid && value == port.written)
                    continue;
                SCHED_STAT(if (_edgeLateOpen) {
                    _edgeLateOpen = false;
                    const int32_t lateUs = (int32_t)(now - _edgeAtUs);
                    if (lateUs > (int32_t)kLateUs)
                        _stats.late++;
                    if (lateUs > 0 && (uint32_t)lateUs > _stats.maxLateUs)
                        _stats.maxLateUs = lateUs;
                });
                _edgeBuf[0] = port.reg;
                _edgeBuf[1] = value;
                startTransfer(Job::Edge, port.address, _edgeBuf, 2, nullptr, 0, 4 * _edgeUs + kRecoverMarginUs);
                _xPorts = p;
                return true;
            }
            return false;
        }

        void SCHED_RAM(ScheduledWire::startSync)(uint32_t now, bool forced)
        {
            const uint8_t ports = mergeOutputWrite(_syncAddress, _txHead, _syncTxLen); // PWM state fixed until the end
            SCHED_STAT(if (!_inStress) {
                const uint32_t waitUs = now - _syncSinceUs;
                _stats.transfers++;
                _stats.waitUs += waitUs;
                if (waitUs > _stats.maxWaitUs)
                    _stats.maxWaitUs = waitUs;
                if (forced)
                    _stats.forced++;
            });
            (void)forced;
            _syncState = kSyncRunning;
            startTransfer(Job::Sync, _syncAddress, _txHead, _syncTxLen, _rxBuf, _syncRxLen, _syncLimitUs);
            _xPorts = ports;
        }

        void SCHED_RAM(ScheduledWire::startInput)()
        {
            const Input& input = _inputs[_inputNext];
            if (_inputStep == 0)
            {
                _inputTx = kInputRegister;
                startTransfer(Job::Input, input.address, &_inputTx, 1, &_inputRx, 1, 4 * transferUs(1, 1) + kRecoverMarginUs);
            }
            else
            {
                _inputTx = kCommandAfterRead;
                startTransfer(Job::Input, input.address, &_inputTx, 1, nullptr, 0, 4 * transferUs(1, 0) + kRecoverMarginUs);
            }
        }

        void SCHED_RAM(ScheduledWire::startQueued)()
        {
            Queued& q = _queue[_queueSend];
            if (q.state == kQueueWaiting)
            {
                q.state = kQueueFirst;
                startTransfer(Job::Queued, q.address, q.first, q.firstLen, nullptr, 0, 4 * transferUs(q.firstLen, 0) + kRecoverMarginUs);
            }
            else
                startTransfer(Job::Queued, q.address, q.second, q.secondLen, nullptr, 0, 4 * transferUs(q.secondLen, 0) + kRecoverMarginUs);
        }

        // End of the transfer on the bus (STOP_DET, or recover()); runs exactly once per transfer.
        void SCHED_RAM(ScheduledWire::complete)(bool ok)
        {
            if (_hwUp)
                _inst->hw->intr_mask = 0;
            const Job job = _job;
            _job = Job::None;
            const uint32_t now = nowUs();
            switch (job)
            {
                case Job::Edge:
                {
                    Port& port = _ports[_xPorts];
                    port.written = _edgeBuf[1];
                    port.writtenValid = ok; // failed: retried on the next event
                    if (ok)
                        port.failures = 0;
                    else
                    {
                        SCHED_STAT(_stats.edgeErrors++);
                        if (++port.failures >= kHoldFailures)
                        {
                            port.failures = 0;
                            port.holdUntilUs = now + kHoldUs;
                            port.holding = true;
                        }
                    }
                    break;
                }
                case Job::Sync:
                    commitOutputWrite(_xPorts, ok);
                    _syncResult = ok ? 0 : (_xTimeout ? 5 : 4);
                    __dmb(); // result visible before the state on the other core
                    SCHED_STAT(if (!_inStress) {
                        _stats.transferUs += now - _xStartUs;
                        if (!ok)
                            _stats.transferErrors++;
                    });
                    _syncState = kSyncDone;
                    break;
                case Job::Input:
                {
                    const Input& input = _inputs[_inputNext];
                    if (_inputStep == 0)
                    {
                        SCHED_STAT(_stats.inputReads++);
                        if (ok)
                        {
                            _inputWord[_inputNext] = kInputValid | _inputRx;
                            _inputUs[_inputNext] = now;
                            _inputFailing &= ~(1 << _inputNext);
                        }
                        else
                            _inputFailing |= 1 << _inputNext;
                        if (input.commandAfterRead)
                        {
                            _inputStep = 1;
                            break;
                        }
                    }
                    _inputStep = 0;
                    if (++_inputNext >= _inputCount)
                        _inputDue = false;
                    break;
                }
                case Job::Queued:
                {
                    Queued& q = _queue[_queueSend];
                    if (q.state == kQueueFirst && ok && q.secondLen)
                    {
                        q.state = kQueueSecond;
                        _queueMid = true;
                        _queueMidUs = now;
                        break;
                    }
                    q.state = ok ? kQueueOk : kQueueFailed; // a failed first segment drops the second
                    _queueMid = false;
                    _queueForced = false;
                    _queueSend = (_queueSend + 1) % OPENKNX_I2C_SCHEDULER_QUEUE;
                    _queueWaitUs = now;
                    SCHED_STAT(_stats.queued++; if (!ok) _stats.queueErrors++;);
                    break;
                }
                default:
                    break;
            }
            kick(job == Job::Edge);
        }

        // ---------------------------------------------------------------- alarm side

        bool SCHED_RAM(ScheduledWire::runEvents)()
        {
            const uint32_t state = schedLock();
            const uint32_t now = nowUs();
            const int32_t lateUs = (int32_t)(now - _nextEventUs);
            if (lateUs >= -(int32_t)kSlackUs) // an early wake-up only re-arms
            {
                SCHED_STAT(_stats.events++; if (_job != Job::None) _stats.busyRetries++;
                           _edgeAtUs = _nextEventUs; _edgeLateOpen = _startBlocks;); // late is counted at the write start
                advanceEvent(now, _startBlocks || (_job == Job::None && !_fallback)); // a transfer runs over a free start: dimmed LEDs wait one period
                _edgeDue = true;
                _edgePort = 0;
            }
            _nextEventUs = _offIndex < _offCount ? _periodStartUs + _off[_offIndex].atUs : _periodStartUs + kPeriodUs;
            _startBlocks = _offCount > 0 || dimmedLeds(); // static LEDs: the next period start does not hold other transfers back
            _nextEventBlocks = _startBlocks;
            pollInputs(now);
            watchdog(now);
            kick(true);
            SCHED_STAT(_stats.edgeWriteUs += nowUs() - now);
            schedUnlock(state);
            return true;
        }

        void SCHED_RAM(ScheduledWire::pollInputs)(uint32_t now)
        {
            const uint8_t count = _inputCount;
            if (!count)
                return;
            for (uint8_t i = 0; i < count; i++)
                if (_hwUp && !_fallback && (_inputWord[i] & kInputValid) && !(_inputFailing & (1 << i)) && now - _inputUs[i] > kInputMaxAgeUs)
                    _inputWord[i] = 0; // reads starve on a working bus: callers read the chip themselves
            if (!_inputDue && now - _inputLastUs >= kInputPeriodUs)
            {
                _inputDue = true;
                _inputNext = 0;
                _inputStep = 0;
                _inputLastUs = now;
            }
        }

        // A transfer past its limit is aborted only after a further window without IRQ progress, so a
        // transfer that only looks overdue because interrupts were off (flash write) gets its pending IRQs first.
        void SCHED_RAM(ScheduledWire::watchdog)(uint32_t now)
        {
            if (_job == Job::None || now - _xStartUs <= _xLimitUs)
                return;
            if (!_wdArmed || _wdProgress != _xProgress)
            {
                _wdArmed = true;
                _wdProgress = _xProgress;
                _wdSinceUs = now;
                return;
            }
            if (now - _wdSinceUs <= kRecoverMarginUs)
                return;
            const uint32_t startUs = _xStartUs;
            service(); // its end may be pending behind this IRQ (catch-up after interrupts were off)
            if (_job != Job::None && _xStartUs == startUs && _wdProgress == _xProgress)
                recover(); // still the same transfer, still no progress
        }

        // Abort the transfer on the bus; reset the block and clear the bus when the abort does not end it.
        void ScheduledWire::recover()
        {
            if (_job == Job::None || !_hwUp)
                return;
            SCHED_STAT(_stats.recoveries++);
            i2c_hw_t* hw = _inst->hw;
            hw->intr_mask = 0;
            hw->enable = I2C_IC_ENABLE_ENABLE_BITS | I2C_IC_ENABLE_ABORT_BITS;
            const uint32_t byteUs = 9000000UL / (_clkHz >= 1000 ? _clkHz : 1000) + 1;
            const uint32_t start = nowUs();
            while ((hw->enable & I2C_IC_ENABLE_ABORT_BITS) && nowUs() - start < 3 * byteUs)
                tight_loop_contents();
            while ((hw->status & I2C_IC_STATUS_ACTIVITY_BITS) && nowUs() - start < 4 * byteUs)
                tight_loop_contents(); // STOP after the abort
            if ((hw->enable & I2C_IC_ENABLE_ABORT_BITS) || (hw->status & I2C_IC_STATUS_ACTIVITY_BITS))
                resetBus();
            else
                (void)hw->clr_intr;
            _xTimeout = true;
            complete(false);
        }

        // Block reset, up to 9 SCL pulses while a device holds SDA low, STOP, then the block set up again.
        void ScheduledWire::resetBus()
        {
            constexpr uint sda = OPENKNX_GPIO_SDA;
            constexpr uint scl = OPENKNX_GPIO_SCL;
            constexpr uint32_t halfUs = 5;
            i2c_deinit(_inst);
            gpio_put(sda, 0); // open drain: output low or input with the pull-up
            gpio_put(scl, 0);
            gpio_set_dir(sda, GPIO_IN);
            gpio_set_dir(scl, GPIO_IN);
            gpio_set_function(sda, GPIO_FUNC_SIO);
            gpio_set_function(scl, GPIO_FUNC_SIO);
            for (uint8_t i = 0; i < 9 && !gpio_get(sda); i++)
            {
                gpio_set_dir(scl, GPIO_OUT);
                busy_wait_us_32(halfUs);
                gpio_set_dir(scl, GPIO_IN);
                busy_wait_us_32(halfUs);
            }
            gpio_set_dir(scl, GPIO_OUT); // STOP: SDA rises while SCL is high
            busy_wait_us_32(halfUs);
            gpio_set_dir(sda, GPIO_OUT);
            busy_wait_us_32(halfUs);
            gpio_set_dir(scl, GPIO_IN);
            busy_wait_us_32(halfUs);
            gpio_set_dir(sda, GPIO_IN);
            busy_wait_us_32(halfUs);
            i2c_init(_inst, _clkHz);
            _inst->hw->intr_mask = 0;
            gpio_set_function(sda, GPIO_FUNC_I2C);
            gpio_set_function(scl, GPIO_FUNC_I2C);
            _engineProfile = false;
        }

        // ---------------------------------------------------------------- caller side

        bool ScheduledWire::interruptible() const
        {
            uint32_t primask;
            __asm volatile("mrs %0, primask" : "=r"(primask));
            return !(primask & 1) && __get_current_exception() == 0;
        }

        // Queue one transfer for the engine and wait for its end; TwoWire result code (0, 4, 5).
        uint8_t ScheduledWire::runSync(uint8_t address, uint8_t txLen, uint8_t rxLen)
        {
            const bool poll = !interruptible(); // IRQs cannot run here: drive the engine from this loop
            uint32_t state = schedLock();
            if (_syncState != kSyncIdle || (poll && _fallback))
            {
                schedUnlock(state);
                return 4; // the previous transfer never ended, or the interrupted code holds the bus
            }
            _rxLen = 0; // nothing to read after a failed transfer, like TwoWire
            _rxOff = 0;
            _rxOwn = true;
            if (address > 0x7F)
            {
                schedUnlock(state);
                return 4;
            }
            const uint32_t timeoutMs = getTimeout() < kMaxTimeoutMs ? getTimeout() : kMaxTimeoutMs;
            _syncAddress = address;
            _syncTxLen = txLen;
            _syncRxLen = rxLen;
            _syncEstUs = transferUs(txLen, rxLen);
            _syncLimitUs = timeoutMs * 1000UL + kRecoverMarginUs;
            _syncPoll = poll;
            _syncSinceUs = nowUs();
            _syncState = kSyncQueued;
            kick(false);
            schedUnlock(state);

            const uint32_t deadlineUs = 2 * kPeriodUs + _syncLimitUs + kRecoverMarginUs;
            while (_syncState != kSyncDone)
            {
                if (poll)
                {
                    state = schedLock();
                    service();
                    kick(false);
                    schedUnlock(state);
                }
                else
                    tight_loop_contents(); // the I2C IRQ and the alarm end the transfer meanwhile
                if (nowUs() - _syncSinceUs > deadlineUs)
                {
                    state = schedLock();
                    if (_syncState == kSyncQueued)
                    {
                        _syncResult = 5;
                        _syncState = kSyncDone;
                    }
                    else if (_syncState == kSyncRunning)
                        recover(); // ends it with 5
                    schedUnlock(state);
                }
            }
            __dmb();
            const uint8_t result = _syncResult;
            if (result == 0)
                _rxLen = rxLen; // a write leaves nothing to read, like TwoWire
            _syncState = kSyncIdle;
            return result;
        }

        // Pause the engine for a TwoWire polling transfer that ends before the next edge (or after one period).
        // False only when IRQs cannot run here and the interrupted code holds the bus.
        bool ScheduledWire::enterFallback(uint32_t durationUs, bool driverTransfer)
        {
            const bool poll = !interruptible();
            const uint32_t start = nowUs();
            for (;;)
            {
                const uint32_t state = schedLock();
                if (poll && _fallback)
                {
                    schedUnlock(state);
                    return false;
                }
                if (poll)
                {
                    service();
                    if (_queueMid && _job == Job::None && _hwUp)
                        startQueued(); // the alarm cannot run here: the second segment goes without a gap check
                }
                kick(false); // pending LED edges go first
                const uint32_t now = nowUs();
                if (_job == Job::None && !_fallback && !_queueMid) // not between the two segments of a queued write
                {
                    const bool fit = poll || fits(now, durationUs);
                    if (fit || now - start > kPeriodUs)
                    {
                        _fallback = true;
                        if (_engineProfile)
                            pollingProfile();
                        SCHED_STAT(if (driverTransfer && !_inStress) {
                            _stats.transfers++;
                            if (now - start > _stats.maxWaitUs)
                                _stats.maxWaitUs = now - start;
                            if (!fit)
                                _stats.forced++;
                            _stats.waitUs += now - start;
                            _grantUs = now;
                            _grantCounted = true;
                        });
                        (void)driverTransfer;
                        schedUnlock(state);
                        return true;
                    }
                }
                schedUnlock(state);
            }
        }

        void ScheduledWire::exitFallback(bool ok)
        {
            if (_hwUp) // a TwoWire read returns before its STOP is out
            {
                const uint32_t start = nowUs();
                const uint32_t limitUs = 2000000UL / (_clkHz >= 1000 ? _clkHz : 1000) + 10;
                while ((_inst->hw->status & I2C_IC_STATUS_ACTIVITY_BITS) && nowUs() - start < limitUs)
                    tight_loop_contents();
            }
            const uint32_t state = schedLock();
            SCHED_STAT(if (_grantCounted) {
                _stats.transferUs += nowUs() - _grantUs;
                _grantCounted = false;
                if (!ok)
                    _stats.transferErrors++;
            });
            (void)ok;
            _fallback = false;
            kick(false);
            schedUnlock(state);
        }

        // Write with STOP on TwoWire's polling path: probe, long write, or no engine.
        uint8_t ScheduledWire::fallbackWrite()
        {
            // TwoWire's probe bit-bangs with sleep_us() steps: about 90 us at 400 kHz, estimate with margin
            if (!enterFallback(_txLen ? transferUs(_txLen, 0) : 3 * transferUs(0, 0), true))
                return 4;
            _rxOwn = false;
            uint8_t ports = 0;
            if (!_txLong)
            {
                const uint32_t state = schedLock();
                ports = mergeOutputWrite(_txAddress, _txHead, _txLen);
                schedUnlock(state);
                TwoWire::beginTransmission(_txAddress);
                forwardHead();
            }
            _inTransfer = true;
            const uint8_t ret = TwoWire::endTransmission(true);
            _inTransfer = false;
            if (ret != 0)
            {
                _rxOwn = true; // TwoWire keeps the written bytes in its buffer after a timeout
                _rxLen = 0;
                _rxOff = 0;
            }
            const uint32_t state = schedLock();
            commitOutputWrite(ports, ret == 0);
            schedUnlock(state);
            exitFallback(ret == 0);
            return ret;
        }

        // Read (after the pending write) on TwoWire's polling path.
        size_t ScheduledWire::fallbackRead(uint8_t address, size_t len, bool writeFirst)
        {
            if (!enterFallback(transferUs(writeFirst ? _txLen : 0, len), true))
                return 0;
            _rxOwn = false;
            uint8_t ports = 0;
            bool ok = true;
            _inTransfer = true;
            if (writeFirst)
            {
                if (!_txLong)
                {
                    const uint32_t state = schedLock();
                    ports = mergeOutputWrite(_txAddress, _txHead, _txLen);
                    schedUnlock(state);
                    TwoWire::beginTransmission(_txAddress);
                    forwardHead();
                }
                ok = TwoWire::endTransmission(false) == 0;
            }
            const size_t got = ok ? TwoWire::requestFrom(address, len, true) : 0;
            if (!ok)
            {
                _rxOwn = true; // TwoWire keeps the written bytes in its buffer after a failed write
                _rxLen = 0;
                _rxOff = 0;
            }
            _inTransfer = false;
            const uint32_t state = schedLock();
            commitOutputWrite(ports, ok);
            schedUnlock(state);
            exitFallback(got == len);
            return got;
        }

        // ---------------------------------------------------------------- TwoWire

        void ScheduledWire::beginTransmission(uint8_t address)
        {
            if (_deferredWrite)
                transmit(); // endTransmission(false) without requestFrom(): send that write with STOP first
            if (_txOpen && _txLong)
                return; // TwoWire holds the open long write and ignores a new start itself
            _txAddress = address;
            _txLen = 0;
            _txOpen = true;
            _txLong = false;
        }

        size_t ScheduledWire::write(uint8_t data)
        {
            if (!_txOpen)
                return 0; // no transmission open, like TwoWire
            if (!_txLong)
            {
                if (_txLen < sizeof(_txHead))
                {
                    _txHead[_txLen++] = data;
                    return 1;
                }
                TwoWire::beginTransmission(_txAddress); // longer than the engine takes: TwoWire collects it
                forwardHead();
                _txLong = true;
            }
            const size_t n = TwoWire::write(data);
            if (n)
                _txLen++;
            return n;
        }

        void ScheduledWire::forwardHead()
        {
            const size_t count = _txLen < sizeof(_txHead) ? _txLen : sizeof(_txHead);
            for (size_t i = 0; i < count; i++)
                TwoWire::write(_txHead[i]);
        }

        uint8_t ScheduledWire::transmit()
        {
            _deferredWrite = false;
            _txOpen = false;
            const uint8_t ret = (_txLong || _txLen == 0 || !_engine || !_hwUp) ? fallbackWrite() : runSync(_txAddress, (uint8_t)_txLen, 0);
            _txLen = 0;
            _txLong = false;
            return ret;
        }

        uint8_t ScheduledWire::endTransmission(bool stopBit)
        {
            if (!_txOpen)
                return 4; // no transmission open, like TwoWire
            if (!stopBit)
            {
                _deferredWrite = true; // sent by the following requestFrom() in the same transfer
                _txOpen = false;
                return 0;
            }
            return transmit();
        }

        size_t ScheduledWire::requestFrom(uint8_t address, size_t len, bool stopBit)
        {
            (void)stopBit; // every read ends with STOP, see ScheduledWire.h
            if (_txOpen)
                return 0; // like TwoWire during an open transmission
            const bool writeFirst = _deferredWrite;
            if (len == 0)
            {
                if (writeFirst)
                    transmit();
                return 0;
            }
            _deferredWrite = false;
            size_t got;
            if (_txLong || len > sizeof(_rxBuf) || (writeFirst && address != _txAddress) || !_engine || !_hwUp)
                got = fallbackRead(address, len, writeFirst);
            else
                got = runSync(address, writeFirst ? (uint8_t)_txLen : 0, (uint8_t)len) == 0 ? len : 0;
            if (writeFirst)
            {
                _txLen = 0;
                _txLong = false;
            }
            return got;
        }

        int ScheduledWire::available()
        {
            return _rxOwn ? _rxLen - _rxOff : TwoWire::available();
        }

        int ScheduledWire::read()
        {
            if (!_rxOwn)
                return TwoWire::read();
            return _rxOff < _rxLen ? _rxBuf[_rxOff++] : -1;
        }

        int ScheduledWire::peek()
        {
            if (!_rxOwn)
                return TwoWire::peek();
            return _rxOff < _rxLen ? _rxBuf[_rxOff] : -1;
        }

        void ScheduledWire::setClock(uint32_t freqHz)
        {
            if (freqHz == _clkHz)
                return; // drivers set the same clock before every transfer
            if (_inTransfer)
            {
                TwoWire::setClock(freqHz); // timeout recovery inside a transfer: the block is already owned
                return;
            }
            if (!enterFallback(OPENKNX_I2C_SCHEDULER_OVERHEAD_US, false))
                return;
            _inTransfer = true;
            TwoWire::setClock(freqHz);
            _inTransfer = false;
            _clkHz = freqHz;
            updateTiming();
            exitFallback(true);
        }

        // ---------------------------------------------------------------- queued writes

        bool SCHED_RAM(ScheduledWire::queueWrite)(uint8_t address, const uint8_t* first, uint8_t firstLen, const uint8_t* second, uint8_t secondLen, uint32_t tag)
        {
            if (address > 0x7F || firstLen == 0 || firstLen > sizeof(Queued::first) || secondLen > sizeof(Queued::second))
                return false;
            if (_queueCount >= OPENKNX_I2C_SCHEDULER_QUEUE) // count and head change only in this caller's queueWrite/queueResult
            {
                SCHED_STAT(const uint32_t state = schedLock(); _stats.queueFull++; schedUnlock(state););
                return false;
            }
            Queued& q = _queue[_queueHead]; // free slot: the engine reads it only after it is published below
            q.tag = tag;
            q.address = address;
            q.firstLen = firstLen;
            q.secondLen = secondLen;
            memcpy(q.first, first, firstLen);
            if (secondLen)
                memcpy(q.second, second, secondLen);
            const uint32_t state = schedLock();
            if (_queueHead == _queueSend)
                _queueWaitUs = nowUs(); // nothing waiting before it
            q.state = kQueueWaiting;
            _queueHead = (_queueHead + 1) % OPENKNX_I2C_SCHEDULER_QUEUE;
            _queueCount++;
            kick(false);
            schedUnlock(state);
            return true;
        }

        bool SCHED_RAM(ScheduledWire::queueResult)(uint32_t& tag, bool& ok)
        {
            const uint32_t state = schedLock();
            Queued& q = _queue[_queueTail];
            const bool done = _queueCount && (q.state == kQueueOk || q.state == kQueueFailed);
            if (done)
            {
                tag = q.tag;
                ok = q.state == kQueueOk;
                q.state = kQueueFree;
                _queueTail = (_queueTail + 1) % OPENKNX_I2C_SCHEDULER_QUEUE;
                _queueCount--;
            }
            schedUnlock(state);
            return done;
        }

        // ---------------------------------------------------------------- input cache

        void ScheduledWire::watchInputs(uint8_t address, bool commandAfterRead)
        {
            const uint32_t state = schedLock();
            const uint8_t count = _inputCount;
            if (count < OPENKNX_I2C_SCHEDULER_MAX_INPUTS)
            {
                _inputs[count] = {address, commandAfterRead};
                _inputWord[count] = 0;
                _inputCount = count + 1;
            }
            schedUnlock(state);
        }

        bool ScheduledWire::cachedInputs(uint8_t address, uint16_t& value) const
        {
            const uint8_t count = _inputCount;
            for (uint8_t i = 0; i < count; i++)
            {
                if (_inputs[i].address != address)
                    continue;
                const uint32_t word = _inputWord[i];
                if (!(word & kInputValid))
                    return false;
                value = (uint16_t)word;
                return true;
            }
            return false;
        }
    } // namespace I2C
} // namespace OpenKNX

#endif // OPENKNX_I2C_SCHEDULER && ARDUINO_ARCH_RP2040
