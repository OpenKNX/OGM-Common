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
            , _lastProcessTime(0)
            , _scheduleCounter(0)
            , _fastTransfersCompleted(0)
            , _slowTransfersCompleted(0)
            , _fastQueueOverflows(0)
            , _slowQueueOverflows(0)
            , _poolExhausted(0)
#endif
        {
#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
            // Initialize Fast Queue
            _fastQueue.head = 0;
            _fastQueue.tail = 0;
            
            // Initialize Slow Queue
            _slowQueue.head = 0;
            _slowQueue.tail = 0;
            
            // Initialize Large Buffer Pool
            for (uint8_t i = 0; i < LARGE_POOL_SIZE; i++)
            {
                _largePool.used[i] = false;
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
            // === Two-Tier Async Queue Mode ===
            
            // Empty transfer (OLED command bytes) - always direct
            if (_txLen == 0)
            {
                int res = _pioi2c->write_blocking(_address, _txBuffer, 0, !stop);
                return (res < 0) ? 4 : 0;
            }
            
            // Detect transfer type and route to appropriate queue
            const bool isDisplayTransfer = (_address == 0x3C || _address == 0x3D);
            const bool isSmallTransfer = (_txLen <= 12);
            
            if (isSmallTransfer && !isDisplayTransfer)
            {
                // === Fast Queue (LED, GPIO expander, sensors) ===
                // Assume first byte is register address for typical I2C devices
                uint8_t reg = _txBuffer[0];
                const uint8_t* data = (_txLen > 1) ? &_txBuffer[1] : nullptr;
                uint8_t dataLen = (_txLen > 1) ? (_txLen - 1) : 0;
                
                bool queued = enqueueFast(_address, reg, data, dataLen, true);
                _txLen = 0;
                return queued ? 0 : 4;
            }
            else if (!isSmallTransfer && isDisplayTransfer)
            {
                // === Slow Queue (OLED large transfers) ===
                // OLED: First byte is usually 0x40 (data) or 0x00 (command)
                uint8_t reg = _txBuffer[0];
                const uint8_t* data = (_txLen > 1) ? &_txBuffer[1] : nullptr;
                uint16_t dataLen = (_txLen > 1) ? (_txLen - 1) : 0;
                
                bool queued = enqueueSlow(_address, reg, data, dataLen, true);
                _txLen = 0;
                return queued ? 0 : 4;
            }
            else
            {
                // === Direct-Write Fallback (edge cases) ===
                // - Display small transfers (command sequences)
                // - Unexpected large non-display transfers
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
        // Two-Tier Async Queue Implementation
        // ========================================

        /*
         * @brief Allocate a large buffer from pool
         * @return Pool index (0-3) or -1 if pool exhausted
         */
        int PIOI2CWire::allocateLargeBuffer()
        {
            for (uint8_t i = 0; i < LARGE_POOL_SIZE; i++)
            {
                if (!_largePool.used[i])
                {
                    _largePool.used[i] = true;
                    return i;
                }
            }
            _poolExhausted++;
            return -1;  // Pool exhausted
        }

        /*
         * @brief Release a large buffer back to pool
         * @param poolIndex Index to release (0-3)
         */
        void PIOI2CWire::releaseLargeBuffer(uint8_t poolIndex)
        {
            if (poolIndex < LARGE_POOL_SIZE)
            {
                _largePool.used[poolIndex] = false;
            }
        }

        /*
         * @brief Enqueue fast transfer (≤12 bytes, ISR-safe)
         * @param address I2C device address
         * @param reg Register address (optional)
         * @param data Data buffer
         * @param length Data length (0-12 bytes)
         * @param hasReg Whether register address is valid
         * @return true if enqueued, false if queue full
         */
        bool PIOI2CWire::enqueueFast(uint8_t address, uint8_t reg, const uint8_t* data, uint8_t length, bool hasReg)
        {
            // Validate length
            if (length > 12)
            {
                logErrorP("enqueueFast: length %d > 12 bytes", length);
                return false;
            }
            
            // Read head/tail with acquire barrier
            DMB_ACQUIRE();
            uint8_t head = _fastQueue.head;
            uint8_t tail = _fastQueue.tail;
            
            // Check if queue full (leave 1 slot empty to distinguish full/empty)
            uint8_t nextHead = (head + 1) & FAST_QUEUE_MASK;
            if (nextHead == tail)
            {
                _fastQueueOverflows++;
                return false;  // Queue full
            }
            
            // Write entry (non-volatile local copy for performance)
            FastQueueEntry entry;
            entry.address = address;
            entry.reg = reg;
            entry.length = length;
            entry.hasReg = hasReg;
            
            // Copy data inline
            if (data != nullptr && length > 0)
            {
                memcpy(entry.data, data, length);
            }
            
            // Write to queue buffer
            memcpy((void*)&_fastQueue.buffer[head], &entry, sizeof(FastQueueEntry));
            
            // Release barrier before updating head
            DMB_RELEASE();
            _fastQueue.head = nextHead;
            
            return true;
        }

        /*
         * @brief Enqueue slow transfer (>12 bytes, ISR-safe)
         * @param address I2C device address
         * @param reg Register address (optional)
         * @param data Data buffer
         * @param length Data length (>12 bytes)
         * @param hasReg Whether register address is valid
         * @return true if enqueued, false if queue/pool full
         */
        bool PIOI2CWire::enqueueSlow(uint8_t address, uint8_t reg, const uint8_t* data, uint16_t length, bool hasReg)
        {
            // Validate length
            if (length <= 12)
            {
                logErrorP("enqueueSlow: length %d <= 12 bytes (use enqueueFast)", length);
                return false;
            }
            if (length > LARGE_BUFFER_SIZE)
            {
                logErrorP("enqueueSlow: length %d > %d bytes", length, LARGE_BUFFER_SIZE);
                return false;
            }
            
            // Allocate large buffer FIRST (before queue entry)
            int poolIndex = allocateLargeBuffer();
            if (poolIndex < 0)
            {
                return false;  // Pool exhausted
            }
            
            // Copy data to pool buffer
            if (data != nullptr && length > 0)
            {
                memcpy(_largePool.buffers[poolIndex], data, length);
            }
            
            // Read head/tail with acquire barrier
            DMB_ACQUIRE();
            uint8_t head = _slowQueue.head;
            uint8_t tail = _slowQueue.tail;
            
            // Check if queue full
            uint8_t nextHead = (head + 1) & SLOW_QUEUE_MASK;
            if (nextHead == tail)
            {
                // Queue full - release buffer and fail
                releaseLargeBuffer(poolIndex);
                _slowQueueOverflows++;
                return false;
            }
            
            // Write entry
            SlowQueueEntry entry;
            entry.address = address;
            entry.reg = reg;
            entry.poolIndex = poolIndex;
            entry.length = length;
            entry.hasReg = hasReg;
            
            // Write to queue buffer
            memcpy((void*)&_slowQueue.buffer[head], &entry, sizeof(SlowQueueEntry));
            
            // Release barrier before updating head
            DMB_RELEASE();
            _slowQueue.head = nextHead;
            
            return true;
        }

        /*
         * @brief Process queues with time-slicing and fair scheduling
         * Call from Main Loop! Max 2ms execution time per call.
         * Fair scheduling: 4:1 Fast:Slow ratio (4× Fast, then 1× Slow)
         */
        void PIOI2CWire::processQueue()
        {
            if (!_pioi2c)
                return;
            
            const uint32_t MAX_PROCESS_TIME_US = 2000;  // 2ms time-slice
            const uint32_t startTime = micros();
            
            // === Fast Queue Processing (4 out of 5 cycles) ===
            if (_scheduleCounter < 4)
            {
                // Acquire barrier before reading head
                DMB_ACQUIRE();
                uint8_t head = _fastQueue.head;
                uint8_t tail = _fastQueue.tail;
                
                // Process Fast Queue entries until time-slice exhausted or queue empty
                while (tail != head)
                {
                    // Check time budget
                    if ((micros() - startTime) >= MAX_PROCESS_TIME_US)
                        break;
                    
                    // Read entry (volatile -> local copy)
                    FastQueueEntry entry;
                    memcpy(&entry, (void*)&_fastQueue.buffer[tail], sizeof(FastQueueEntry));
                    
                    // Build I2C transfer buffer
                    uint8_t buffer[13];  // Max: 1 reg + 12 data
                    uint8_t bufLen = 0;
                    
                    if (entry.hasReg)
                    {
                        buffer[bufLen++] = entry.reg;
                    }
                    
                    if (entry.length > 0)
                    {
                        memcpy(&buffer[bufLen], entry.data, entry.length);
                        bufLen += entry.length;
                    }
                    
                    // Execute I2C write (blocking, but fast <200µs for LED)
                    int result = _pioi2c->write_blocking(entry.address, buffer, bufLen, false);
                    
                    if (result >= 0)
                    {
                        _fastTransfersCompleted++;
                    }
                    
                    // Release barrier before updating tail
                    DMB_RELEASE();
                    _fastQueue.tail = (tail + 1) & FAST_QUEUE_MASK;
                    
                    // Acquire barrier before reading updated head/tail
                    DMB_ACQUIRE();
                    head = _fastQueue.head;
                    tail = _fastQueue.tail;
                }
            }
            // === Slow Queue Processing (1 out of 5 cycles) ===
            else
            {
                // Acquire barrier before reading head
                DMB_ACQUIRE();
                uint8_t head = _slowQueue.head;
                uint8_t tail = _slowQueue.tail;
                
                // Process Slow Queue entries until time-slice exhausted or queue empty
                while (tail != head)
                {
                    // Check time budget
                    if ((micros() - startTime) >= MAX_PROCESS_TIME_US)
                        break;
                    
                    // Read entry
                    SlowQueueEntry entry;
                    memcpy(&entry, (void*)&_slowQueue.buffer[tail], sizeof(SlowQueueEntry));
                    
                    // Build I2C transfer buffer (reg + large data from pool)
                    uint8_t* poolBuffer = _largePool.buffers[entry.poolIndex];
                    uint8_t buffer[LARGE_BUFFER_SIZE + 1];  // Max: 1 reg + 1024 data
                    uint16_t bufLen = 0;
                    
                    if (entry.hasReg)
                    {
                        buffer[bufLen++] = entry.reg;
                    }
                    
                    if (entry.length > 0)
                    {
                        memcpy(&buffer[bufLen], poolBuffer, entry.length);
                        bufLen += entry.length;
                    }
                    
                    // Execute I2C write (blocking, ~8ms for OLED full-screen)
                    int result = _pioi2c->write_blocking(entry.address, buffer, bufLen, false);
                    
                    if (result >= 0)
                    {
                        _slowTransfersCompleted++;
                    }
                    
                    // Release large buffer back to pool
                    releaseLargeBuffer(entry.poolIndex);
                    
                    // Release barrier before updating tail
                    DMB_RELEASE();
                    _slowQueue.tail = (tail + 1) & SLOW_QUEUE_MASK;
                    
                    // Acquire barrier before reading updated head/tail
                    DMB_ACQUIRE();
                    head = _slowQueue.head;
                    tail = _slowQueue.tail;
                }
            }
            
            // Update fair scheduling counter (0-4, wraps to 0 after 4)
            _scheduleCounter = (_scheduleCounter + 1) % 5;
            
            // Store timestamp for next call
            _lastProcessTime = micros();
        }

        /*
         * @brief Flush display queue (priority flush for widget-switch)
         * Forces immediate processing of Slow Queue (OLED transfers)
         * Call BEFORE widget switch to prevent pool overflow!
         */
        void PIOI2CWire::flushDisplayQueue()
        {
            if (!_pioi2c)
                return;
            
            // Acquire barrier
            DMB_ACQUIRE();
            uint8_t head = _slowQueue.head;
            uint8_t tail = _slowQueue.tail;
            
            // Process all Slow Queue entries (no time-slice limit!)
            while (tail != head)
            {
                // Read entry
                SlowQueueEntry entry;
                memcpy(&entry, (void*)&_slowQueue.buffer[tail], sizeof(SlowQueueEntry));
                
                // Build I2C transfer buffer
                uint8_t* poolBuffer = _largePool.buffers[entry.poolIndex];
                uint8_t buffer[LARGE_BUFFER_SIZE + 1];
                uint16_t bufLen = 0;
                
                if (entry.hasReg)
                {
                    buffer[bufLen++] = entry.reg;
                }
                
                if (entry.length > 0)
                {
                    memcpy(&buffer[bufLen], poolBuffer, entry.length);
                    bufLen += entry.length;
                }
                
                // Execute I2C write (blocking)
                int result = _pioi2c->write_blocking(entry.address, buffer, bufLen, false);
                
                if (result >= 0)
                {
                    _slowTransfersCompleted++;
                }
                
                // Release large buffer
                releaseLargeBuffer(entry.poolIndex);
                
                // Release barrier before updating tail
                DMB_RELEASE();
                _slowQueue.tail = (tail + 1) & SLOW_QUEUE_MASK;
                
                // Acquire barrier before reading updated head/tail
                DMB_ACQUIRE();
                head = _slowQueue.head;
                tail = _slowQueue.tail;
            }
        }
        
#endif // OPENKNX_I2C_USE_ASYNC_QUEUE

    } // namespace I2C
} // namespace OpenKNX
#endif // defined(ARDUINO_ARCH_RP2040)