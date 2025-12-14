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
            // Initialize Single Queue
            _queue.head = 0; // Queue head index (next write position)
            _queue.tail = 0; // Queue tail index (next read position)
            _queue.count = 0; // Current number of entries in the queue
            memset(_queue.buffer, 0, sizeof(_queue.buffer)); // Clear queue entries
            memset(_txBuffer, 0, sizeof(_txBuffer)); // Clear TX buffer
            memset(_rxBuffer, 0, sizeof(_rxBuffer)); // Clear RX buffer
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

    #if defined(OPENKNX_I2C_USE_SPINLOCK)
            // Initialize spinlock once (shared across all instances)
            if (_i2cLock == nullptr)
            {
                _i2cLock = spin_lock_init(spin_lock_claim_unused(true));
                logInfoP("Hardware spinlock enabled for ISR-safe I2C (try-lock mode)");
            }
    #endif

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
            size_t toCopy = (quantity < (sizeof(_txBuffer) - _txLen)) ? quantity : (sizeof(_txBuffer) - _txLen);
            memcpy(_txBuffer + _txLen, data, toCopy);
            _txLen += toCopy;
            return toCopy;
        }

        /*
         * @brief End transmission to I2C slave device
         * @param stop Whether to send a STOP condition after transmission
         * @return 0 on success, 4 on error
         */
        uint8_t PIOI2CWire::endTransmission(bool stop)
        {
            if (!_pioi2c) return 4;

    #if defined(OPENKNX_I2C_USE_ASYNC_QUEUE)
            // === Single Queue Mode ===

            // Validate length (fallback for edge cases >29 bytes - extremely rare)
            if (_txLen > MAX_ENTRY_DATA)
            {
                // Blocking fallback for oversized transfers (should never happen in practice)
                int res = _pioi2c->write_blocking(_address, _txBuffer, _txLen, !stop);
                _txLen = 0;
                return (res < 0) ? 4 : 0;
            }

            // Enqueue to single queue with stop flag
            bool queued = enqueue(_address, _txBuffer, _txLen, stop);
            _txLen = 0;

            return queued ? 0 : 4; // 0 = success, 4 = queue full

    #elif defined(OPENKNX_I2C_USE_SPINLOCK)
            // Hardware spinlock with non-blocking try-lock for ISR safety
            // If called from ISR and Display holds lock: skip update, retry in 10ms
            // If called from main thread: this will succeed immediately (no contention from ISR)
            if (!spin_try_lock_unsafe(_i2cLock))
            {
                // Lock busy (Display using I2C) - skip this update
                _tryLockFailCount++;
                if (_tryLockFailCount % 100 == 0)
                {
                    logDebugP("I2C try-lock failed %lu times (success: %lu)", _tryLockFailCount, _tryLockSuccessCount);
                }
                return 4; // I2C busy error
            }

            _tryLockSuccessCount++;
            int res = _pioi2c->write_blocking(_address, _txBuffer, _txLen, !stop);
            spin_unlock_unsafe(_i2cLock);
            _txLen = 0;
            return (res < 0) ? 4 : 0;

    #else
            // Simple blocking mode (PENDING_PATTERN or no safety)
            // Note: PENDING_PATTERN logic is handled in LED::GPIO layer,
            // not here in I2C driver. LEDs set _hasPendingI2C flag,
            // then flushPendingI2C() calls this method from main loop.
            int res = _pioi2c->write_blocking(_address, _txBuffer, _txLen, !stop);
            _txLen = 0;
            return (res < 0) ? 4 : 0;
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

    #if defined(OPENKNX_I2C_USE_SPINLOCK)
            // Hardware spinlock - blocking OK here (not called from ISR)
            uint32_t saved_irq = spin_lock_blocking(_i2cLock);
            int res = _pioi2c->_read_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm, address, data, length);
            spin_unlock(_i2cLock, saved_irq);
            return res;
    #else
            // Direct blocking read (no busy flag needed with queue)
            return _pioi2c->_read_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm, address, data, length);
    #endif
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

    #if defined(OPENKNX_I2C_USE_SPINLOCK)
            // Hardware spinlock - blocking OK here (not called from ISR)
            uint32_t saved_irq = spin_lock_blocking(_i2cLock);
            int res = _pioi2c->_write_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm, address, data, length);
            spin_unlock(_i2cLock, saved_irq);
            return res;
    #else
            // Direct blocking write (no busy flag needed with queue)
            return _pioi2c->_write_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm, address, data, length);
    #endif
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

    #if defined(OPENKNX_I2C_USE_SPINLOCK)
            // Try non-blocking lock first (ISR-safe)
            if (!spin_try_lock_unsafe(_i2cLock))
            {
                // I2C bus busy - skip this read if called from ISR
                _tryLockFailCount++;

                // Debug logging every 100 failures
                if (_tryLockFailCount % 100 == 0)
                {
                    logDebugP("I2C requestFrom: spin_try_lock failed %lu times (success: %lu)",
                              _tryLockFailCount, _tryLockSuccessCount);
                }
                return 0;
            }
            _tryLockSuccessCount++;

            int res = _pioi2c->read_blocking(address, _rxBuffer, quantity, !stop);
            spin_unlock_unsafe(_i2cLock);

            if (res == 0)
            {
                _rxLen = quantity;
                _rxPos = 0;
                return quantity;
            }
            _rxLen = _rxPos = 0;
            return 0;
    #else
            // Direct blocking read (no busy flag needed with queue)
            int res = _pioi2c->read_blocking(address, _rxBuffer, quantity, !stop);

            if (res == 0)
            {
                _rxLen = quantity;
                _rxPos = 0;
                return quantity;
            }
            _rxLen = _rxPos = 0;
            return 0;
    #endif
        }

    #if defined(OPENKNX_I2C_USE_SPINLOCK)
        /*
         * @brief Try to acquire I2C lock (non-blocking, ISR-safe)
         * @return true if lock acquired, false if busy
         */
        bool PIOI2CWire::tryLockI2C()
        {
            if (!spin_try_lock_unsafe(_i2cLock))
            {
                _tryLockFailCount++;
                if (_tryLockFailCount % 100 == 0)
                {
                    logDebugP("I2C tryLock failed %lu times (success: %lu)",
                              _tryLockFailCount, _tryLockSuccessCount);
                }
                return false;
            }
            _tryLockSuccessCount++;
            return true;
        }

        /*
         * @brief Release I2C lock
         */
        void PIOI2CWire::unlockI2C()
        {
            spin_unlock_unsafe(_i2cLock);
        }

        /*
         * @brief Internal requestFrom without lock (for use within locked sections)
         * @param address I2C slave address
         * @param quantity Number of bytes to request
         * @param stop Whether to send STOP after read
         * @return Number of bytes read
         */
        size_t PIOI2CWire::requestFrom_locked(uint8_t address, size_t quantity, bool stop)
        {
            if (!_pioi2c) return 0;

            int res = _pioi2c->read_blocking(address, _rxBuffer, quantity, !stop);

            if (res == 0)
            {
                _rxLen = quantity;
                _rxPos = 0;
                return quantity;
            }
            _rxLen = _rxPos = 0;
            return 0;
        }
    #endif

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
         * @brief Enqueue I2C transfer to single queue (ISR-safe)
         * @param address I2C device address
         * @param data Data buffer to send
         * @param length Data length (1-29 bytes)
         * @param send_stop Send STOP condition (false = Repeated START)
         * @return true if enqueued, false if queue full
         */
        bool PIOI2CWire::enqueue(uint8_t address, const uint8_t* data, uint16_t length, bool send_stop)
        {
            // Validate length
            if (length == 0 || length > MAX_ENTRY_DATA)
            {
                return false; // Invalid length
            }

            // Atomic queue-full check using count (leave 1 slot empty)
            DMB_ACQUIRE();
            uint16_t currentCount = _queue.count;
            if (currentCount >= (QUEUE_SIZE - 1))
            {
                _queueOverflows++;
                return false; // Queue full
            }

            // Get current head
            uint16_t head = _queue.head;
            uint16_t nextHead = (head + 1) & QUEUE_MASK;

            // Write entry (direct assignment for optimal speed)
            _queue.buffer[head].address = address;
            _queue.buffer[head].length = length;
            _queue.buffer[head].send_stop = send_stop;

            // Fast memcpy for data
            if (data != nullptr)
            {
                memcpy(_queue.buffer[head].data, data, length);
            }

            // Release barrier before updating head and incrementing count
            DMB_RELEASE();
            _queue.head = nextHead;
            _queue.count++; // Increment count atomically

            return true;
        }

        /*
         * @brief Process queue with DMA-aware batching
         * Call from Main Loop! Max 10 entries per call (~500µs blocking time)
         * With DMA: non-blocking, returns immediately
         * Without DMA: blocking per entry (~50µs each)
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

            // Acquire barrier before reading count
            DMB_ACQUIRE();
            uint16_t currentCount = _queue.count;

            // Early exit if queue empty
            if (currentCount == 0)
                return;

            // Track peak queue usage
            if (currentCount > _queuePeakCount)
            {
                _queuePeakCount = currentCount;
            }

            // Adaptive processing limit
            uint16_t maxEntriesThisCall = MAX_ENTRIES_PER_CALL;
            if (currentCount > (QUEUE_SIZE / 2))
            {
              //  maxEntriesThisCall = MAX_ENTRIES_PER_CALL/2;
                //logDebugP("High I2C queue load (%d entries) - reducing processing batch size", currentCount);
            } 
            else if (currentCount > (QUEUE_SIZE / 4)) 
            {
                // maxEntriesThisCall = (MAX_ENTRIES_PER_CALL * 3)/4;
                //logDebugP("Moderate I2C queue load (%d entries) - reducing processing batch size", currentCount);
            }

            uint16_t processed = 0;
            uint16_t tail = _queue.tail;

            while (currentCount > 0 && processed < maxEntriesThisCall)
            {
                QueueEntry& entry = _queue.buffer[tail];

        #ifdef OPENKNX_PIO_I2C_DMA
                if (_pioi2c->_dma_available)
                {
                    if (dma_channel_is_busy(_pioi2c->_dma_tx))
                    {
                        // DMA busy, fallback to blocking
                        _pioi2c->_write_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm,
                                                 entry.address, entry.data, entry.length, entry.send_stop);
                        _blockingTransfers++;
                    }
                    else
                    {
                        // Use DMA
                        _pioi2c->_write_dma(_pioi2c->_inst->pio, _pioi2c->_inst->sm,
                                            entry.address, entry.data, entry.length, entry.send_stop);
                        _dmaTransfers++;
                    }
                }
                else
        #endif
                {
                    // Blocking mode
                    _pioi2c->_write_blocking(_pioi2c->_inst->pio, _pioi2c->_inst->sm,
                                             entry.address, entry.data, entry.length, entry.send_stop);
                    _blockingTransfers++;
                }

                _transfersCompleted++;
                processed++;

                // Watchdog feeding
                if (processed > 0 && (processed % 3) == 0)
                    watchdog_update();

                // Release barrier before updating tail and decrementing count
                DMB_RELEASE();
                _queue.tail = (tail + 1) & QUEUE_MASK;
                _queue.count--; // Decrement count atomically

                // Acquire barrier before reading updated count for next iteration
                DMB_ACQUIRE();
                currentCount = _queue.count;
                tail = _queue.tail;
            }

            _totalEntriesProcessed += processed;
        }

    #endif // OPENKNX_I2C_USE_ASYNC_QUEUE

    } // namespace I2C
} // namespace OpenKNX
#endif // defined(ARDUINO_ARCH_RP2040)