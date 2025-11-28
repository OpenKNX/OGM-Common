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
#if defined(OPENKNX_I2C_USE_SPINLOCK)
        spin_lock_t* PIOI2CWire::_i2cLock = nullptr; // Initialize static spinlock
        volatile uint32_t PIOI2CWire::_tryLockFailCount = 0;
        volatile uint32_t PIOI2CWire::_tryLockSuccessCount = 0;
#elif !defined(OPENKNX_I2C_USE_ASYNC_QUEUE)
        bool PIOI2CWire::_i2cBusy = false; // Initialize static busy flag (not used with queue)
#endif
        /*
         * @brief Construct a new PIO I2C Wire object
         * @param sda_pin SDA pin number
         * @param scl_pin SCL pin number
         * @param baudrate I2C clock speed in Hz (default 100000)
         */
        PIOI2CWire::PIOI2CWire(uint32_t sda_pin, uint32_t scl_pin, uint32_t baudrate)
            : TwoWire(i2c0, sda_pin, scl_pin), // Übergebe i2c0 (wird nicht benutzt, aber nötig)
              _sda(sda_pin),
              _scl(scl_pin),
              _baudrate(baudrate),
              _pioi2c(nullptr),
              _txLen(0),
              _rxLen(0),
              _rxPos(0),
              _address(0)
#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
            , _queueHead(0)
            , _queueTail(0)
            , _queueCount(0)
            , _transferBusy(false)
            , _completedHead(0)
            , _completedTail(0)
            , _completedCount(0)
            , _transfersCompleted(0)
            , _transfersFailed(0)
            , _queueOverflows(0)
#endif
        {
#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
            // Initialize queue entries
            for (uint8_t i = 0; i < OPENKNX_I2C_QUEUE_SIZE; i++)
            {
                _queue[i].active = false;
                _queue[i].callback = nullptr;
                _completedQueue[i].active = false;
                _completedQueue[i].callback = nullptr;
            }
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
            // === Async Queue Mode ===
            // Queue ALL small I2C transfers (except Display which uses empty command transfers)
            // PCA9557 now uses cached state - no RMW dependency, queue-safe!
            
            if (_txLen == 0)
            {
                // Empty transfer - only Display uses this for command bytes
                int res = _pioi2c->write_blocking(_address, _txBuffer, 0, !stop);
                return (res < 0) ? 4 : 0;
            }
            
            // Queue small transfers, direct blocking for large
            const bool isDisplayTransfer = (_address == 0x3C || _address == 0x3D);
            const bool isLargeTransfer = (_txLen > 32);
            
            if (!isDisplayTransfer && !isLargeTransfer)
            {
                // Queue it (includes PCA9557 with cached state)
                bool queued = enqueueWrite(_address, _txBuffer, _txLen, nullptr);
                _txLen = 0;
                return queued ? 0 : 4;
            }
            else
            {
                // Display or large transfer: Direct blocking
                int res = _pioi2c->write_blocking(_address, _txBuffer, _txLen, !stop);
                _txLen = 0;
                return (res < 0) ? 4 : 0;
            }
            
#elif defined(OPENKNX_I2C_USE_SPINLOCK)
            // Hardware spinlock with non-blocking try-lock for ISR safety
            // If called from ISR and Display holds lock: skip update, retry in 10ms
            // If called from main thread: this will succeed immediately (no contention from ISR)
            if (!spin_try_lock_unsafe(_i2cLock)) {
                // Lock busy (Display using I2C) - skip this update
                _tryLockFailCount++;
                if (_tryLockFailCount % 100 == 0) {
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
            // Simple blocking mode (no ISR safety)
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
        // Async Queue Implementation
        // ========================================

        /*
         * @brief Enqueue I2C write transfer (ISR-safe, non-blocking)
         * @param address I2C device address
         * @param data Pointer to data buffer
         * @param length Data length in bytes (max 32 for inline buffer)
         * @param callback Completion callback (optional, executed in Main Loop!)
         * @return true if enqueued successfully, false if queue full or data too large
         */
        bool PIOI2CWire::enqueueWrite(uint8_t address, const uint8_t* data, uint8_t length, void (*callback)(bool))
        {
            // Sanity checks
            if (length == 0 || length > 32 || data == nullptr)
            {
                return false;
            }

            // Check queue overflow
            if (_queueCount >= OPENKNX_I2C_QUEUE_SIZE)
            {
                _queueOverflows++;
                return false;
            }

            // Atomic enqueue (ISR-safe)
            uint32_t saveInterrupts = save_and_disable_interrupts();

            I2CQueueEntry& entry = const_cast<I2CQueueEntry&>(_queue[_queueTail]);
            entry.address = address;
            memcpy((void*)entry.data, data, length);  // Copy to inline buffer
            entry.length = length;
            entry.type = TransferType::WRITE;
            entry.callback = callback;
            entry.active = true;

            _queueTail = (_queueTail + 1) % OPENKNX_I2C_QUEUE_SIZE;
            _queueCount++;

            restore_interrupts(saveInterrupts);

            return true;
        }

        /*
         * @brief Process queue - starts next transfer if idle
         * MUST be called from Main Loop!
         */
        void PIOI2CWire::processQueue()
        {
            // Check if queue empty or transfer busy
            if (_queueCount == 0 || _transferBusy)
            {
                return;
            }

            // Atomic queue access
            uint32_t saveInterrupts = save_and_disable_interrupts();

            I2CQueueEntry& transfer = const_cast<I2CQueueEntry&>(_queue[_queueHead]);

            if (!transfer.active)
            {
                restore_interrupts(saveInterrupts);
                return;
            }

            // Mark as busy
            _transferBusy = true;

            // Copy transfer data for processing (minimize atomic section)
            uint8_t address = transfer.address;
            uint8_t data[32];
            memcpy(data, (void*)transfer.data, transfer.length);
            uint16_t length = transfer.length;
            TransferType type = transfer.type;
            void (*callback)(bool) = transfer.callback;

            restore_interrupts(saveInterrupts);

            // Execute I2C transfer (blocking, but we're in Main Loop)
            int result = 0;
            if (type == TransferType::WRITE)
            {
                result = WriteBlocking(address, data, length);
            }
            // TODO: Add READ and WRITE_READ support

            bool success = (result >= 0);

            // Transfer complete - update queue
            saveInterrupts = save_and_disable_interrupts();

            // Queue callback if provided
            if (callback != nullptr && _completedCount < OPENKNX_I2C_QUEUE_SIZE)
            {
                CompletedTransfer& completed = const_cast<CompletedTransfer&>(_completedQueue[_completedTail]);
                completed.callback = callback;
                completed.success = success;
                completed.active = true;

                _completedTail = (_completedTail + 1) % OPENKNX_I2C_QUEUE_SIZE;
                _completedCount++;
            }

            // Update stats
            if (success)
            {
                _transfersCompleted++;
            }
            else
            {
                _transfersFailed++;
            }

            // Dequeue transfer
            transfer.active = false;
            _queueHead = (_queueHead + 1) % OPENKNX_I2C_QUEUE_SIZE;
            _queueCount--;

            // Mark as idle
            _transferBusy = false;

            restore_interrupts(saveInterrupts);
        }

        /*
         * @brief Process completed callbacks
         * MUST be called from Main Loop! Callbacks can use I2C, malloc, Serial, etc.
         */
        void PIOI2CWire::processCallbacks()
        {
            while (_completedCount > 0)
            {
                // Atomic access
                uint32_t saveInterrupts = save_and_disable_interrupts();

                CompletedTransfer& completed = const_cast<CompletedTransfer&>(_completedQueue[_completedHead]);

                if (!completed.active)
                {
                    restore_interrupts(saveInterrupts);
                    break;
                }

                // Copy callback info (minimize atomic section)
                void (*callback)(bool) = completed.callback;
                bool success = completed.success;

                // Mark as processed
                completed.active = false;
                _completedHead = (_completedHead + 1) % OPENKNX_I2C_QUEUE_SIZE;
                _completedCount--;

                restore_interrupts(saveInterrupts);

                // Execute callback OUTSIDE atomic section (Main Loop context!)
                if (callback != nullptr)
                {
                    callback(success);
                }
            }
        }
#endif // OPENKNX_I2C_USE_ASYNC_QUEUE

    } // namespace I2C
} // namespace OpenKNX
#endif // defined(ARDUINO_ARCH_RP2040)