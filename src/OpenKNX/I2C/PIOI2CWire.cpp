#if defined(ARDUINO_ARCH_RP2040) // PIO I2C is only available on RP2040/RP2350 platforms
    #include "PIOI2CWire.h"
    #include "OpenKNX/Facade.h"
    #include <Arduino.h>
    #include <pico/stdlib.h>
    #include <string.h>

namespace OpenKNX
{
    namespace I2C
    {
        /*
         * @brief Construct a new PIO I2C Wire object
         * @param sda_pin SDA pin number
         * @param scl_pin SCL pin number
         * @param baudrate I2C clock speed in Hz (default 100000)
         */
        PIOI2CWire::PIOI2CWire(uint32_t sda_pin, uint32_t scl_pin, uint32_t baudrate)
            : TwoWire(i2c0, sda_pin, scl_pin), // Übergebe i2c0 (wird nicht benutzt, aber nötig)
              _sda(sda_pin),                   // SDA pin number
              _scl(scl_pin),                   // SCL pin number
              _baudrate(baudrate),             // I2C clock speed in Hz
              _pioi2c(nullptr),                // PIO I2C instance pointer
              _txLen(0),                       // Transmit length
              _rxLen(0),                       // Receive length
              _rxPos(0),                       // Receive position
              _address(0)                      // I2C address
    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
              ,
              _transfersCompleted(0),    // Transfers completed count
              _queueOverflows(0),        // Queue overflow count
              _queuePeakCount(0),        // Peak queue count
              _totalProcessCalls(0),     // Total process calls
              _totalEntriesProcessed(0), // Total entries processed
              _dmaTransfers(0),          // DMA transfers count
              _blockingTransfers(0),     // Blocking transfers count
              _lastStatsUpdate(0)        // Last statistics update time
    #endif
        {
    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
            _queue.head = 0;                                 // Queue head index (next write position)
            _queue.tail = 0;                                 // Queue tail index (next read position)
            memset(_queue.buffer, 0, sizeof(_queue.buffer)); // Clear queue entries
            memset(_txBuffer, 0, sizeof(_txBuffer));         // Clear TX buffer
            memset(_rxBuffer, 0, sizeof(_rxBuffer));         // Clear RX buffer
    #endif
        }

        /*
         * @brief Initialize the PIO I2C bus
         * @error codes:
         * [E001] - Memory allocation failed
         *        Cause: Heap exhausted | Fix: Free memory or increase heap size
         * [E002] - Invalid pin configuration
         *        Cause: SCL pin is not SDA+1 | Fix: Use adjacent GPIO pins (e.g. 26/27, 4/5)
         * [E003] - GPIO base mismatch (RP2350B only)
         *        Cause: Pins outside PIO's GPIO range | Fix: Check pio_set_gpio_base() configuration
         */
        void PIOI2CWire::begin()
        {
            logDebugP("begin called");

            if (_pioi2c != nullptr && _pioi2c->_inst)
            {
                logDebugP("Already initialized - skipping begin.");
                logDebugP("Current PIO I2C: PIO%d-SM%d @ %dkHz",
                          pio_get_index(_pioi2c->get_pio()),
                          _pioi2c->get_sm(),
                          _baudrate / 1000);
                return;
            }

            if (_pioi2c != nullptr)
            {
                if (_pioi2c->_inst)
                {
                    pio_sm_set_enabled(_pioi2c->_inst->pio, _pioi2c->_inst->sm, false);
                    pio_sm_clear_fifos(_pioi2c->_inst->pio, _pioi2c->_inst->sm);
                    pio_interrupt_clear(_pioi2c->_inst->pio, _pioi2c->_inst->sm);
                }
                delete _pioi2c;
                _pioi2c = nullptr;
            }

            gpio_init(_sda);
            gpio_init(_scl);
            gpio_set_function(_sda, GPIO_FUNC_NULL);
            gpio_set_function(_scl, GPIO_FUNC_NULL);
            gpio_set_dir(_sda, GPIO_IN);
            gpio_set_dir(_scl, GPIO_IN);
            gpio_pull_up(_sda);
            gpio_pull_up(_scl);
            // sleep_ms(50); // Allow lines to stabilize, if needed.We can uncomment this if we see issues.

            logDebugP("Creating PIO I2C (SDA:%d SCL:%d)...", _sda, _scl);
            _pioi2c = new pio_i2c(_sda, _scl, _baudrate);

            if (_pioi2c && _pioi2c->_inst)
            {
                // Push the configured timeout down so wait_idle()/put16() use it.
                _pioi2c->set_timeout_us(_timeout_ms * 1000UL);

                logDebugP("[OK] PIO%d-SM%d @ %dkHz",
                          pio_get_index(_pioi2c->get_pio()),
                          _pioi2c->get_sm(),
                          _baudrate / 1000);
    #ifdef OPENKNX_PIO_I2C_DMA
                if (_pioi2c->_dma_available)
                {
                    logInfoP("DMA enabled (TX:%d RX:%d) - Non-blocking I2C active",
                             _pioi2c->_dma_tx, _pioi2c->_dma_rx);
                }
                else
                {
                    logWarningP("DMA unavailable - Using blocking I2C mode");
                }
    #endif
            }
            else
            {
                if (!_pioi2c)
                    logErrorP("[E001] Out of memory");
                else if (_scl != _sda + 1)
                    logErrorP("[E002] SCL must be SDA+1 (got %d/%d)", _sda, _scl);
                else
                    logErrorP("[E003] No PIO/SM available (max 2 buses)");
            }
        }

        /*
         * @brief Shut down the PIO I2C bus
         */
        void PIOI2CWire::end()
        {
            if (_pioi2c)
            {
                if (_pioi2c->_inst) // Stop and cleanup the state machine
                {
                    PIO pio = _pioi2c->_inst->pio;
                    uint sm = _pioi2c->_inst->sm;

                    pio_sm_set_enabled(pio, sm, false); // Disable SM
                    pio_sm_clear_fifos(pio, sm);        // Clear FIFOs
                    pio_interrupt_clear(pio, sm);       // Clear interrupts

                    gpio_set_function(_sda, GPIO_FUNC_NULL); // Disable PIO control for SDA
                    gpio_set_function(_scl, GPIO_FUNC_NULL); // Disable PIO control for SCL
                    gpio_set_dir(_sda, GPIO_IN);             // Set SDA to input
                    gpio_set_dir(_scl, GPIO_IN);             // Set SCL to input
                }

                delete _pioi2c;    // Delete the pio_i2c instance
                _pioi2c = nullptr; // Clear pointer
            }
        }

        /*
         * @brief Set I2C clock speed
         * @param baudrate I2C clock speed in Hz
         */
        void PIOI2CWire::setClock(uint32_t baudrate)
        {
            _baudrate = baudrate;
            if (_pioi2c)
            {
                _pioi2c->set_baudrate(baudrate);
            }
        }

        /*
         * @brief Begin transmission to I2C slave device
         * @param address I2C slave address
         */
        void PIOI2CWire::beginTransmission(uint8_t address)
        {
            _address = address;
            _txLen = 0;
            _txOverflow = false; // Reset "data too long" flag for this transmission
        }

        /*
         * @brief Write a byte to the I2C bus
         * @param data Byte to write
         * @return Number of bytes written (1 on success, 0 on failure)
         */
        size_t PIOI2CWire::write(uint8_t data)
        {
            if (_txLen < sizeof(_txBuffer))
            {
                _txBuffer[_txLen++] = data;
                return 1;
            }
            _txOverflow = true; // overflow -> endTransmission returns 1 (data too long)
            return 0;
        }

        /*
         * @brief Write multiple bytes to the I2C bus
         * @param data Pointer to data buffer
         * @param quantity Number of bytes to write
         * @return Number of bytes written
         */
        size_t PIOI2CWire::write(const uint8_t* data, size_t quantity)
        {
            size_t space = sizeof(_txBuffer) - _txLen;
            size_t toCopy = (quantity < space) ? quantity : space;
            memcpy(_txBuffer + _txLen, data, toCopy);
            _txLen += toCopy;
            if (toCopy < quantity)
                _txOverflow = true; // not all bytes fit -> report "data too long"
            return toCopy;
        }

        /*
         * @brief Map a pio_i2c transfer result (0/-1/-2/-3) to an Arduino TwoWire
         *        endTransmission() status code (0/4/2/3).
         */
        static inline uint8_t pioResultToTwoWire(int res)
        {
            switch (res)
            {
                case pio_i2c::PIO_I2C_OK: return 0;            // success
                case pio_i2c::PIO_I2C_ERR_ADDR_NACK: return 2; // NACK on address
                case pio_i2c::PIO_I2C_ERR_DATA_NACK: return 3; // NACK on data
                default: return 4;                             // other error
            }
        }

        /*
         * @brief End transmission to I2C slave device (Arduino TwoWire semantics).
         *
         * Return codes: 0 success, 1 data too long, 2 addr NACK, 3 data NACK, 4 other.
         *
         * Sync/async rule (top-to-bottom): overflow -> 1; probe / repeated-START / small write
         * (<= ASYNC_MIN_LEN) / oversized (> MAX_ENTRY_DATA) all run SYNC and return the real code;
         * otherwise fire-and-forget async queue returning 0 = "queued". Sync on repeated-START
         * guarantees the reg-pointer write is on the wire before the caller's requestFrom.
         * With ASYNC_MIN_LEN >= MAX_ENTRY_DATA the async band is empty (dormant here), but
         * processQueue() still serves LED/probe enqueues.
         */
        uint8_t PIOI2CWire::endTransmission(bool stop)
        {
            if (!_pioi2c) return 4;

            // Data that never fit the TX buffer is reported as "data too long" (1), not sent short.
            if (_txOverflow)
            {
                _txLen = 0;
                _txOverflow = false;
                return 1;
            }

    #if defined(OPENKNX_I2C_USE_ASYNC_QUEUE)
            // Probe / repeated-START / small / oversized writes run SYNC under the SM ownership
            // flag and return the real code; only larger stop-terminated writes go async below.
            const bool runSync = (_txLen == 0) ||             // probe
                                 (!stop) ||                   // repeated-START, read follows
                                 (_txLen <= ASYNC_MIN_LEN) || // small register/command
                                 (_txLen > MAX_ENTRY_DATA);   // oversized, unqueueable
            if (runSync)
            {
                if (!tryLockSM()) return 4; // SM busy elsewhere: report other error, no spin
                int res = _pioi2c->write_blocking(_address, _txBuffer, _txLen, !stop);
                unlockSM();
                _txLen = 0;
                uint8_t code = pioResultToTwoWire(res);
                _lastTxStatus = code;
                return code;
            }

            // Fire-and-forget: enqueue and return 0 == "queued" (not wire-confirmed); full -> 4.
            bool queued = enqueue(_address, _txBuffer, _txLen, stop);
            _txLen = 0;
            if (!queued) return 4;
            return 0; // queued; see lastAsyncError()/asyncPending()

    #else
            // Simple blocking mode, under the SM ownership flag for race-safety.
            if (!tryLockSM()) return 4; // SM busy: report other error rather than corrupt
            int res = _pioi2c->write_blocking(_address, _txBuffer, _txLen, !stop);
            unlockSM();
            _txLen = 0;
            uint8_t code = pioResultToTwoWire(res);
            _lastTxStatus = code;
            return code;
    #endif
        }

        /*
         * @brief Blocking read method for direct access
         * @param address I2C slave address
         * @param data Pointer to data buffer
         * @param length Number of bytes to read
         * @return Number of bytes read, or -1 on error
         */
        int PIOI2CWire::ReadBlocking(uint8_t address, uint8_t* data, size_t length)
        {
            if (!_pioi2c) return -1;

            // Take SM ownership so a concurrent processQueue() drain can't touch the FIFO mid-read.
            if (!tryLockSM()) return -1;
            int res = _pioi2c->_read_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm, address, data, length);
            // Generic (non-NACK) error is the wedged-bus signature -> recover.
            if (res == pio_i2c::PIO_I2C_ERR_OTHER) _pioi2c->bus_recovery();
            unlockSM();
            _lastTxStatus = pioResultToTwoWire(res);
            return res;
        }

        /*
         * @brief Blocking write method for direct access
         * @param address I2C slave address
         * @param data Pointer to data buffer
         * @param length Number of bytes to write
         * @return Number of bytes written, or -1 on error
         */
        int PIOI2CWire::WriteBlocking(uint8_t address, uint8_t* data, size_t length)
        {
            if (!_pioi2c) return -1;

            // Same SM ownership flag as reads/queue drain.
            if (!tryLockSM()) return -1; // SM busy: don't corrupt an in-flight transfer
            int res = _pioi2c->_write_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm, address, data, length);
            // Generic (non-NACK) error -> wedged bus -> 9-clock recovery.
            if (res == pio_i2c::PIO_I2C_ERR_OTHER) _pioi2c->bus_recovery();
            unlockSM();
            _lastTxStatus = pioResultToTwoWire(res);
            return res;
        }

        /*
         * @brief Trigger a 9-clock SDA bus-recovery on a stuck bus.
         * @return true if SDA was released, false otherwise (or not initialised / SM busy).
         */
        bool PIOI2CWire::busRecovery()
        {
            if (!_pioi2c) return false;
            if (!tryLockSM()) return false; // SM busy: skip, retry later (no spin)
            bool ok = _pioi2c->bus_recovery();
            unlockSM();
            return ok;
        }

        /*
         * @brief Request bytes from I2C slave device
         * @param address I2C slave address
         * @param quantity Number of bytes to request
         * @param stop Whether to send a STOP condition after the request
         * @return Number of bytes read
         */
        size_t PIOI2CWire::requestFrom(uint8_t address, size_t quantity, bool stop)
        {
            if (!_pioi2c) return 0;

            // Reads drive the same PIO SM as the queued writes, so serialize via the SM flag.
            if (!tryLockSM()) return 0; // SM busy: no bytes read (non-blocking, no spin)
            int res = _pioi2c->read_blocking(address, _rxBuffer, quantity, !stop);
            // Generic (non-NACK) error -> wedged bus -> 9-clock recovery.
            if (res == pio_i2c::PIO_I2C_ERR_OTHER) _pioi2c->bus_recovery();
            unlockSM();

            _lastTxStatus = pioResultToTwoWire(res);

            if (res == 0)
            {
                _rxLen = quantity;
                _rxPos = 0;
                return quantity;
            }
            _rxLen = _rxPos = 0;
            return 0;
        }

        /*
         * @brief Check available bytes to read
         * @return Number of bytes available to read
         */
        int PIOI2CWire::available()
        {
            return _rxLen - _rxPos;
        }

        /*
         * @brief Read a byte from the I2C bus
         * @return Byte read, or -1 if no data available
         */
        int PIOI2CWire::read()
        {
            if (_rxPos < _rxLen) return _rxBuffer[_rxPos++];
            return -1;
        }

        /*
         * @brief Flush buffers
         */
        void PIOI2CWire::flush()
        {
            _txLen = 0;
            _rxLen = _rxPos = 0;
        }

    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
        // ========================================
        // Single Queue Implementation
        // ========================================

        /*
         * @brief Enqueue an I2C transfer into the SPSC ring (producer side, single context).
         * @param address   I2C device address
         * @param data      Data buffer to send (may be nullptr when length==0)
         * @param length    Data length 0..MAX_ENTRY_DATA. length==0 is a valid address-only probe.
         * @param send_stop Send STOP condition (false = Repeated START)
         * @return true if enqueued; false (counted as overflow) if the queue is full or oversized.
         */
        bool PIOI2CWire::enqueue(uint8_t address, const uint8_t* data, uint16_t length, bool send_stop)
        {
            // Oversize is a caller error -> count as overflow, don't truncate.
            if (length > MAX_ENTRY_DATA)
            {
                _queueOverflows++;
                return false;
            }

            // Acquire tail; leave one slot empty to distinguish full from empty.
            DMB_ACQUIRE();
            uint16_t head = _queue.head;
            uint16_t tail = _queue.tail;
            uint16_t nextHead = (head + 1) & QUEUE_MASK;
            if (nextHead == (tail & QUEUE_MASK))
            {
                _queueOverflows++;
                return false; // Queue full
            }

            // Write entry (direct assignment for optimal speed)
            _queue.buffer[head].address = address;
            _queue.buffer[head].length = length;
            _queue.buffer[head].send_stop = send_stop;

            // Fast memcpy for data (skip for zero-length probes)
            if (length > 0 && data != nullptr)
            {
                memcpy(_queue.buffer[head].data, data, length);
            }

            // Release: publish the entry before advancing head.
            DMB_RELEASE();
            _queue.head = nextHead;

            return true;
        }

        /*
         * @brief Drain the SPSC ring (consumer side). Call from the main loop.
         *
         * The whole drain holds the SM ownership flag (a software token, not interrupts-off, so
         * the watchdog/USB keep running), so it never touches the SM/FIFO concurrently with a
         * read. If a read owns the SM, the drain defers to the next call (no spin). Each drained
         * write's result is stored in _lastTxStatus and surfaced via lastAsyncError().
         */
        void PIOI2CWire::processQueue()
        {
            if (!_pioi2c)
                return;

            _totalProcessCalls++;

        #ifdef OPENKNX_PIO_I2C_DMA
            // Check if DMA is still busy from previous call
            if (_pioi2c->_dma_available && dma_channel_is_busy(_pioi2c->_dma_tx))
            {
                return; // DMA busy, skip this iteration
            }
        #endif

            // Derive occupancy from head/tail (acquire the producer's published head).
            DMB_ACQUIRE();
            uint16_t head = _queue.head;
            uint16_t tail = _queue.tail;
            uint16_t currentCount = (head >= tail) ? (uint16_t)(head - tail)
                                                   : (uint16_t)(QUEUE_SIZE - tail + head);

            // Early exit if queue empty
            if (currentCount == 0)
                return;

            // Track peak queue usage (stat, consumer-owned)
            if (currentCount > _queuePeakCount)
            {
                _queuePeakCount = currentCount;
            }

            const uint16_t maxEntriesThisCall = MAX_ENTRIES_PER_CALL;

            uint16_t processed = 0;

            // Take SM ownership for the whole batch; if a read owns it, defer to the next call.
            if (!tryLockSM())
                return;

            while (currentCount > 0 && processed < maxEntriesThisCall)
            {
                QueueEntry& entry = _queue.buffer[tail];

                int res = 0;

        #ifdef OPENKNX_PIO_I2C_DMA
                if (_pioi2c->_dma_available)
                {
                    if (dma_channel_is_busy(_pioi2c->_dma_tx))
                    {
                        // DMA busy, fallback to blocking
                        res = _pioi2c->_write_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm,
                                                       entry.address, entry.data, entry.length, entry.send_stop);
                        _blockingTransfers++;
                    }
                    else
                    {
                        // Use DMA (fire-and-forget mid-batch; res==0 means no immediate error)
                        res = _pioi2c->_write_dma(_pioi2c->_inst->pio, _pioi2c->_inst->sm,
                                                  entry.address, entry.data, entry.length, entry.send_stop);
                        _dmaTransfers++;
                    }
                }
                else
        #endif
                {
                    // Blocking mode
                    res = _pioi2c->_write_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm,
                                                   entry.address, entry.data, entry.length, entry.send_stop);
                    _blockingTransfers++;
                }

                // Publish this transfer's real result (see lastAsyncError()).
                _lastTxStatus = pioResultToTwoWire(res);

                _transfersCompleted++;
                processed++;

                // Watchdog feeding
                if ((processed % 3) == 0)
                    watchdog_update();

                // Release: advance tail (entry consumed).
                DMB_RELEASE();
                tail = (tail + 1) & QUEUE_MASK;
                _queue.tail = tail;

                // Re-derive occupancy (producer may have added more).
                DMB_ACQUIRE();
                head = _queue.head;
                currentCount = (head >= tail) ? (uint16_t)(head - tail)
                                              : (uint16_t)(QUEUE_SIZE - tail + head);
            }

            unlockSM(); // release SM ownership for reads / next drain

            _totalEntriesProcessed += processed;
        }

    #endif // OPENKNX_I2C_USE_ASYNC_QUEUE

    } // namespace I2C
} // namespace OpenKNX
#endif // defined(ARDUINO_ARCH_RP2040)