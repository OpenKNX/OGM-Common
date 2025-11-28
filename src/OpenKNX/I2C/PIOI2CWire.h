#pragma once
/**
 * @file        PIOI2CWire.h
 * @brief       PIO-based I2C Wire implementation for OpenKNX
 * @version     0.0.1
 * @date        2025-10-30
 * @copyright   Copyright (c) 2025, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 */
#if defined(ARDUINO_ARCH_RP2040) // PIO I2C is only available on RP2040/RP2350 platforms

// === I2C Thread-Safety Options (choose ONE) ===
// Option 1: Hardware spinlock with try-lock (ISR skips if busy - RECOMMENDED for testing)
// #define OPENKNX_I2C_USE_SPINLOCK 1

// Option 2: Pending pattern (ISR sets flag, main loop writes - SAFEST for LED+Display)
#define OPENKNX_I2C_USE_PENDING_PATTERN 1

// Option 3: Async Queue (DMA-based, ISR-safe, non-blocking - BEST for LED+Display)
//#define OPENKNX_I2C_USE_ASYNC_QUEUE 1

    #include "pio/pio_i2c.h"
    #include <Wire.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <string>
#ifdef OPENKNX_I2C_USE_SPINLOCK
    #include <hardware/sync.h> // For spin_lock
#endif

#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
    #ifndef OPENKNX_I2C_QUEUE_SIZE
        #define OPENKNX_I2C_QUEUE_SIZE 128  // 128 × 34 bytes = 4.3 KB (sufficient for LED PWM + safety margin)
    #endif
#endif

namespace OpenKNX
{
    namespace I2C
    {
#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
        // Transfer types for async queue
        enum class TransferType : uint8_t
        {
            WRITE,
            READ,
            WRITE_READ
        };

        // Queue entry for async I2C transfers
        struct I2CQueueEntry
        {
            uint8_t address;
            uint8_t data[32];  // Inline buffer für kleine Transfers (LED = 1 byte)
            uint16_t length;
            TransferType type;
            void (*callback)(bool success);
            bool active;
        };
        
        // Completed transfer for Main Loop callbacks
        struct CompletedTransfer
        {
            void (*callback)(bool success);
            bool success;
            bool active;
        };
#endif

        class PIOI2CWire : public TwoWire // PIO-based I2C Wire implementation, inherits from TwoWire original Wire class
        {
          private:
#if defined(OPENKNX_I2C_USE_SPINLOCK)
            static spin_lock_t* _i2cLock; // Hardware spinlock for ISR-safe I2C access
            static volatile uint32_t _tryLockFailCount; // Debug: Count failed try-locks
            static volatile uint32_t _tryLockSuccessCount; // Debug: Count successful try-locks
#elif defined(OPENKNX_I2C_USE_ASYNC_QUEUE)
            // Async Queue members
            volatile I2CQueueEntry _queue[OPENKNX_I2C_QUEUE_SIZE];
            volatile uint8_t _queueHead;
            volatile uint8_t _queueTail;
            volatile uint8_t _queueCount;
            volatile bool _transferBusy;
            
            volatile CompletedTransfer _completedQueue[OPENKNX_I2C_QUEUE_SIZE];
            volatile uint8_t _completedHead;
            volatile uint8_t _completedTail;
            volatile uint8_t _completedCount;
            
            volatile uint32_t _transfersCompleted;
            volatile uint32_t _transfersFailed;
            volatile uint32_t _queueOverflows;
#else
            static bool _i2cBusy; // Simple busy flag
#endif

          public:
            PIOI2CWire(uint32_t sda_pin, uint32_t scl_pin, uint32_t baudrate = 100000); // Constructor

            void begin() override; // Start as Master. Use default assigned pins
            inline void begin(uint32_t sda_pin, uint32_t scl_pin)
            {
                _sda = sda_pin;
                _scl = scl_pin;
                begin();
            } // Start as Master with specified pins
            inline void begin(uint32_t sda_pin, uint32_t scl_pin, uint32_t baudrate)
            {
                _baudrate = baudrate;
                begin(sda_pin, scl_pin);
            } // Start as Master with specified pins and baudrate
            void end() override; // Shut down the I2C interface

            void setClock(uint32_t baudrate) override; // Set I2C clock speed
            void setTimeout(uint32_t timeout_ms) { _timeout_ms = timeout_ms; } // Set I2C timeout in ms
            inline bool setSDA(pin_size_t sda)
            {
                _sda = sda;
                return true;
            } // Select SDA pin to use. Call before ::begin()
            inline bool setSCL(pin_size_t scl)
            {
                _scl = scl;
                return true;
            } // Select SCL pin to use. Call before ::begin()

            inline uint getPioIndex() { return pio_get_index(_pioi2c->get_pio()); } // Get PIO index
            inline uint getStateMachineNumber() { return _pioi2c->_inst->sm; }      // Get State Machine number
            inline uint getOffset() { return _pioi2c->_inst->prog_offset; }         // Get program offset

            void beginTransmission(uint8_t address) override;           // Begin transmission to I2C slave device
            uint8_t endTransmission(bool stop) override;                // End transmission to I2C slave device
            uint8_t endTransmission() { return endTransmission(true); } // Default to sending stop

            size_t write(uint8_t data) override;                         // Write a byte to the I2C bus
            size_t write(const uint8_t* data, size_t quantity) override; // Write multiple bytes to the I2C bus

            size_t requestFrom(uint8_t address, size_t quantity, bool stop) override;                             // Request bytes from I2C slave device
            size_t requestFrom(uint8_t address, size_t quantity) { return requestFrom(address, quantity, true); } // Default to sending stop

            int available() override; // Check available bytes to read
            int read() override;      // Read a byte from the I2C bus
            void flush() override;    // Flush buffers

            // Additional direct access methods, not part of TwoWire interface

            int ReadBlocking(uint8_t address, uint8_t* data, size_t length);  // Blocking read method for direct access
            int WriteBlocking(uint8_t address, uint8_t* data, size_t length); // Blocking write method for direct access

            inline bool ping(uint8_t address)
            {
                uint8_t dummy;
                return (ReadBlocking(address, &dummy, 1) >= 0);
            } // Ping an I2C device
            inline bool pingw(uint8_t address)
            {
                uint8_t dummy = 0;
                return (WriteBlocking(address, &dummy, 0) >= 0);
            } // Ping an I2C device for write access

            const std::string logPrefix() { return "PIOI2C"; } // Log prefix
            inline pio_i2c* getInstance() { return _pioi2c; }  // Get underlying PIO I2C instances

#if defined(OPENKNX_I2C_USE_SPINLOCK)
            // Atomic I2C operations for ISR-safe multi-step transactions (e.g., Read-Modify-Write)
            bool tryLockI2C();    // Try to acquire I2C lock (non-blocking, ISR-safe)
            void unlockI2C();     // Release I2C lock
            
            // Internal requestFrom without lock (for use within locked sections)
            size_t requestFrom_locked(uint8_t address, size_t quantity, bool stop);
#endif

#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
            // === Async Queue Methods (ISR-Safe) ===
            // Enqueue transfer (ISR-safe, non-blocking)
            bool enqueueWrite(uint8_t address, const uint8_t* data, uint8_t length, void (*callback)(bool) = nullptr);
            
            // Process queue - call from Main Loop!
            void processQueue();
            
            // Process callbacks - call from Main Loop!
            void processCallbacks();
            
            // Queue status
            inline bool isQueueBusy() const { return _transferBusy; }
            inline uint8_t queuedCount() const { return _queueCount; }
            inline uint8_t queueFreeSpace() const { return OPENKNX_I2C_QUEUE_SIZE - _queueCount; }
            
            // Stats
            inline uint32_t getTransfersCompleted() const { return _transfersCompleted; }
            inline uint32_t getTransfersFailed() const { return _transfersFailed; }
            inline uint32_t getQueueOverflows() const { return _queueOverflows; }
#endif

          private:
            pio_i2c* _pioi2c; // Underlying PIO I2C instance
            uint32_t _sda, _scl;
            uint32_t _baudrate;
            uint32_t _timeout_ms = 10; // Default timeout 10ms
            uint8_t _txBuffer[256]; // Important, we need min 256 buffer, especially for i2c Display...
            size_t _txLen;
            uint8_t _rxBuffer[256]; // Important, we need min 256 buffer, especially for i2c Display...
            size_t _rxLen;
            size_t _rxPos;
            uint8_t _address;
        };
    } // namespace I2C
} // namespace OpenKNX
#endif // defined(ARDUINO_ARCH_RP2040)