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
    
    // === Queue Sizes (Power-of-2 for bitwise AND masking) ===
    #define FAST_QUEUE_SIZE 128  // Fast queue for ≤12 byte transfers
    #define FAST_QUEUE_MASK 127  // Bitwise AND mask (SIZE - 1)
    #define SLOW_QUEUE_SIZE 8    // Slow queue for >12 byte transfers
    #define SLOW_QUEUE_MASK 7    // Bitwise AND mask (SIZE - 1)
    #define LARGE_POOL_SIZE 4    // Number of large buffers (1KB each)
    #define LARGE_BUFFER_SIZE 1024  // Size of each large buffer
#endif

namespace OpenKNX
{
    namespace I2C
    {
#ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
        // === Fast Queue Entry (inline data ≤12 bytes) ===
        struct FastQueueEntry
        {
            uint8_t address;      // I2C device address
            uint8_t reg;          // Register address (if applicable)
            uint8_t data[12];     // Inline data buffer
            uint8_t length;       // Data length (0-12 bytes)
            bool hasReg;          // Whether reg is valid
        } __attribute__((packed));  // 16 bytes total
        
        // === Slow Queue Entry (external pool for >12 bytes) ===
        struct SlowQueueEntry
        {
            uint8_t address;      // I2C device address
            uint8_t reg;          // Register address (if applicable)
            uint8_t poolIndex;    // Index into large buffer pool
            uint16_t length;      // Data length (>12 bytes)
            bool hasReg;          // Whether reg is valid
        } __attribute__((packed));  // 6 bytes total (+ 2 padding = 8 bytes)
        
        // === Fast Queue Structure (Cache-Line Aligned) ===
        struct alignas(64) FastQueue
        {
            alignas(64) volatile uint8_t head;  // Producer writes (Core 0 or Core 1)
            uint8_t _pad_head[63];              // Padding to 64-byte cache-line
            
            alignas(64) volatile uint8_t tail;  // Consumer writes (Main Loop)
            uint8_t _pad_tail[63];              // Padding to 64-byte cache-line
            
            alignas(64) FastQueueEntry buffer[FAST_QUEUE_SIZE];  // 128 × 16B = 2048B
        };
        
        // === Slow Queue Structure (Cache-Line Aligned) ===
        struct alignas(64) SlowQueue
        {
            alignas(64) volatile uint8_t head;  // Producer writes
            uint8_t _pad_head[63];              // Padding to 64-byte cache-line
            
            alignas(64) volatile uint8_t tail;  // Consumer writes
            uint8_t _pad_tail[63];              // Padding to 64-byte cache-line
            
            alignas(64) SlowQueueEntry buffer[SLOW_QUEUE_SIZE];  // 8 × 8B = 64B
        };
        
        // === Large Buffer Pool (for OLED full-screen transfers) ===
        struct LargeBufferPool
        {
            uint8_t buffers[LARGE_POOL_SIZE][LARGE_BUFFER_SIZE];  // 4 × 1024B = 4096B
            volatile bool used[LARGE_POOL_SIZE];                   // Pool slot usage flags
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
            // === Async Queue Members (Two-Tier: Fast + Slow) ===
            FastQueue _fastQueue;              // Fast queue for ≤12 byte transfers
            SlowQueue _slowQueue;              // Slow queue for >12 byte transfers
            LargeBufferPool _largePool;        // Pool for large data buffers
            
            // === Processing State ===
            uint32_t _lastProcessTime;         // Timestamp of last processQueue() call
            uint8_t _scheduleCounter;          // Fair scheduling counter (4:1 Fast:Slow)
            
            // === Statistics ===
            volatile uint32_t _fastTransfersCompleted;
            volatile uint32_t _slowTransfersCompleted;
            volatile uint32_t _fastQueueOverflows;
            volatile uint32_t _slowQueueOverflows;
            volatile uint32_t _poolExhausted;
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
            // === Async Queue Methods (Dual-Core Safe) ===
            
            // Enqueue fast transfer (≤12 bytes, ISR-safe)
            bool enqueueFast(uint8_t address, uint8_t reg, const uint8_t* data, uint8_t length, bool hasReg = true);
            
            // Enqueue slow transfer (>12 bytes, ISR-safe)
            bool enqueueSlow(uint8_t address, uint8_t reg, const uint8_t* data, uint16_t length, bool hasReg = true);
            
            // Process queues - call from Main Loop! (max 2ms per call)
            void processQueue();
            
            // Priority flush for display queue (widget-switch safety)
            void flushDisplayQueue();
            
            // === Pool Management ===
            int allocateLargeBuffer();         // Allocate buffer from pool (returns index or -1)
            void releaseLargeBuffer(uint8_t poolIndex);  // Release buffer back to pool
            
            // === Queue Status ===
            inline uint8_t fastQueueCount() const {
                uint8_t h = _fastQueue.head;
                uint8_t t = _fastQueue.tail;
                return (h >= t) ? (h - t) : (FAST_QUEUE_SIZE - t + h);
            }
            inline uint8_t slowQueueCount() const {
                uint8_t h = _slowQueue.head;
                uint8_t t = _slowQueue.tail;
                return (h >= t) ? (h - t) : (SLOW_QUEUE_SIZE - t + h);
            }
            inline uint8_t fastQueueFree() const { return FAST_QUEUE_SIZE - fastQueueCount() - 1; }
            inline uint8_t slowQueueFree() const { return SLOW_QUEUE_SIZE - slowQueueCount() - 1; }
            
            // === Statistics ===
            inline uint32_t getFastTransfersCompleted() const { return _fastTransfersCompleted; }
            inline uint32_t getSlowTransfersCompleted() const { return _slowTransfersCompleted; }
            inline uint32_t getFastQueueOverflows() const { return _fastQueueOverflows; }
            inline uint32_t getSlowQueueOverflows() const { return _slowQueueOverflows; }
            inline uint32_t getPoolExhausted() const { return _poolExhausted; }
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