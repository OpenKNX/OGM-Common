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
//#define OPENKNX_I2C_USE_PENDING_PATTERN 1

// Option 3: Async Queue (Two-Tier, Dual-Core Safe, ISR-safe, non-blocking - BEST for LED+Display)
#define OPENKNX_I2C_USE_ASYNC_QUEUE 1

    #include "pio/pio_i2c.h"
    #include <Wire.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <string>
#ifdef OPENKNX_I2C_USE_SPINLOCK
    #include <hardware/sync.h> // For spin_lock
#endif

#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
    // === Memory Barrier Macros (ARM Cortex-M Dual-Core Safety) ===
    #define DMB_ACQUIRE() __asm__ __volatile__ ("dmb" ::: "memory")  // Load barrier (acquire semantics)
    #define DMB_RELEASE() __asm__ __volatile__ ("dmb" ::: "memory")  // Store barrier (release semantics)
    
    // === Single Queue Configuration (Power-of-2 for bitwise AND masking) ===
    #define QUEUE_SIZE 1024              // Single queue for all transfers (worst-case: 721 display + 30 LEDs)
    #define QUEUE_MASK 1023              // Bitwise AND mask (SIZE - 1)
    #define MAX_ENTRY_DATA 29            // Max inline data per entry (29 bytes)
    #define MAX_ENTRIES_PER_CALL 50      // Process max 50 entries per processQueue() call (~2.5ms @ 400kHz)
#endif

namespace OpenKNX
{
    namespace I2C
    {
#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
        // === Queue Entry (32 bytes, cache-aligned) ===
        struct alignas(32) QueueEntry
        {
            uint8_t address;      // I2C device address
            uint16_t length;      // Data length (1-29 bytes)
            uint8_t data[MAX_ENTRY_DATA];  // Inline data storage (29 bytes)
        };  // Exactly 32 bytes (1 + 2 + 29 = 32)
        
        // === Single Queue Structure (Cache-Line Aligned for Dual-Core Safety) ===
        struct alignas(64) Queue
        {
            alignas(64) volatile uint16_t head;  // Producer writes (ISR or Main Loop)
            uint8_t _pad_head[62];              // Padding to 64-byte cache-line
            
            alignas(64) volatile uint16_t tail;  // Consumer writes (Main Loop only)
            uint8_t _pad_tail[62];              // Padding to 64-byte cache-line
            
            alignas(32) QueueEntry buffer[QUEUE_SIZE];  // 1024 × 32B = 32KB
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
            // === Single Queue Members ===
            Queue _queue;                      // Single queue for all transfers
            
            // === Statistics ===
            volatile uint32_t _transfersCompleted;  // Total transfers completed
            volatile uint32_t _queueOverflows;      // Queue overflow events
            
            // === Extended Performance Metrics ===
            volatile uint16_t _queuePeakCount;      // Peak queue usage (max entries at once)
            volatile uint32_t _totalProcessCalls;   // Total processQueue() calls
            volatile uint32_t _totalEntriesProcessed; // Total entries processed
            volatile uint32_t _dmaTransfers;        // DMA transfers (if available)
            volatile uint32_t _blockingTransfers;   // Blocking transfers (fallback)
            uint32_t _lastStatsUpdate;              // Timestamp of last stats update
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
            // === Single Queue Methods (Dual-Core Safe) ===
            
            // Enqueue transfer (ISR-safe, up to 29 bytes inline)
            bool enqueue(uint8_t address, const uint8_t* data, uint16_t length);
            
            // Process queue - call from Main Loop! (max 10 entries per call = ~500µs)
            void processQueue();
            
            // === Queue Status ===
            inline uint16_t queueCount() const {
                uint16_t h = _queue.head;
                uint16_t t = _queue.tail;
                return (h >= t) ? (h - t) : (QUEUE_SIZE - t + h);
            }
            inline uint16_t queueFree() const { return QUEUE_SIZE - queueCount() - 1; }
            
            // === Basic Statistics ===
            inline uint32_t getTransfersCompleted() const { return _transfersCompleted; }
            inline uint32_t getQueueOverflows() const { return _queueOverflows; }
            
            // === Extended Performance Metrics ===
            inline uint16_t getQueuePeakCount() const { return _queuePeakCount; }
            inline uint32_t getTotalProcessCalls() const { return _totalProcessCalls; }
            inline uint32_t getTotalEntriesProcessed() const { return _totalEntriesProcessed; }
            inline uint32_t getDmaTransfers() const { return _dmaTransfers; }
            inline uint32_t getBlockingTransfers() const { return _blockingTransfers; }
            
            // === Performance Analysis (calculated) ===
            inline float getQueueEfficiency() const {
                // Efficiency: successful transfers / (successful + overflows)
                uint32_t total = _transfersCompleted + _queueOverflows;
                return (total > 0) ? (float)_transfersCompleted / total * 100.0f : 100.0f;
            }
            inline float getAvgEntriesPerCall() const {
                // Average entries processed per processQueue() call
                return (_totalProcessCalls > 0) ? (float)_totalEntriesProcessed / _totalProcessCalls : 0.0f;
            }
            inline float getQueueUtilization() const {
                // Current queue fill percentage
                return (float)queueCount() / QUEUE_SIZE * 100.0f;
            }
            inline float getPeakUtilization() const {
                // Peak queue fill percentage
                return (float)_queuePeakCount / QUEUE_SIZE * 100.0f;
            }
            inline float getDmaRatio() const {
                // Percentage of DMA vs blocking transfers
                uint32_t total = _dmaTransfers + _blockingTransfers;
                return (total > 0) ? (float)_dmaTransfers / total * 100.0f : 0.0f;
            }
            
            // Reset statistics (for benchmarking)
            inline void resetStats() {
                _transfersCompleted = 0;
                _queueOverflows = 0;
                _queuePeakCount = 0;
                _totalProcessCalls = 0;
                _totalEntriesProcessed = 0;
                _dmaTransfers = 0;
                _blockingTransfers = 0;
            }
#endif

          private:
            pio_i2c* _pioi2c; // Underlying PIO I2C instance
            uint32_t _sda, _scl;
            uint32_t _baudrate;
            uint32_t _timeout_ms = 10; // Default timeout 10ms
            uint8_t _txBuffer[2048]; // Match LARGE_BUFFER_SIZE for full display frames
            size_t _txLen;
            uint8_t _rxBuffer[256]; // Important, we need min 256 buffer, especially for i2c Display...
            size_t _rxLen;
            size_t _rxPos;
            uint8_t _address;
        };
    } // namespace I2C
} // namespace OpenKNX
#endif // defined(ARDUINO_ARCH_RP2040)