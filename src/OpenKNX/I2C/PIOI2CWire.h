#pragma once
/**
 * @file        PIOI2CWire.h
 * @brief       PIO-based I2C Wire implementation for OpenKNX
 * @version     0.0.1
 * @date        2025-10-30
 * @copyright   Copyright (c) 2025, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#if defined(ARDUINO_ARCH_RP2040) // PIO I2C is only available on RP2040/RP2350 platforms

// Thread-safety: ASYNC_QUEUE processes transfers via a lock-free ring (optionally DMA).
// #define OPENKNX_I2C_USE_PENDING_PATTERN 1
    #define OPENKNX_I2C_USE_ASYNC_QUEUE 1

    #include "hardware/sync.h" // save_and_disable_interrupts / restore_interrupts (SM critical section)
    #include "pio/pio_i2c.h"
    #include <Wire.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <string>

    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
        // DMB memory barriers for dual-core SPSC ring publish/consume ordering.
        #define DMB_ACQUIRE() __asm__ __volatile__("dmb" ::: "memory")
        #define DMB_RELEASE() __asm__ __volatile__("dmb" ::: "memory")

        // Queue must be power-of-2 for the AND mask.
        #define QUEUE_SIZE 2048   // ~12x full display updates + LEDs
        #define QUEUE_MASK 2047   // SIZE - 1
        #define MAX_ENTRY_DATA 28 // 28 data + 4 header = 32-byte entry
        #define MAX_ENTRIES_PER_CALL 256

        // Writes <= ASYNC_MIN_LEN run SYNC and return the real TwoWire code; larger stop-terminated
        // writes may take the async queue. With ASYNC_MIN_LEN >= MAX_ENTRY_DATA the async band is
        // empty (every write is sync or oversized-blocking); kept named so the boundary is explicit.
        #define ASYNC_MIN_LEN 32
    #endif

    // rx and tx buffer sizes
    #define PIOI2C_DEFAULT_TX_BUFFER_SIZE 1024 // 1KB for display chunks
    #define PIOI2C_DEFAULT_RX_BUFFER_SIZE 256  // 256B for sensor reads
    #define PIOI2C_DEFAULT_TIMEOUT_MS 10       // Default timeout in milliseconds
    #define CACHE_LINE_SIZE 64                 // pad head, tail, count to cache line size (64 bytes) for dual-core safety

    // QueueEntry size must be 32 bytes for cache alignment
    #define QUEUE_ENTRY_SIZE 32                             // 32 bytes per entry for entry alignment and DMA efficiency
    #define QUEUE_DATA_SIZE (QUEUE_SIZE * QUEUE_ENTRY_SIZE) // Total queue data size in bytes

namespace OpenKNX
{
    namespace I2C
    {
    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
        // === Queue Entry (32 bytes, cache-aligned) ===
        struct alignas(QUEUE_ENTRY_SIZE) QueueEntry
        {
            uint16_t length;              // Data length (1-28 bytes) (2 bytes) - first for alignment
            uint8_t address;              // I2C device address (1 byte)
            bool send_stop;               // Send STOP condition (false = Repeated START) (1 byte)
            uint8_t data[MAX_ENTRY_DATA]; // Inline data storage (28 bytes: 29 - 1 for alignment)
        }; // Exactly 32 bytes: 2 + 1 + 1 + 28 = 32 bytes (no padding needed)

        // Lock-free SPSC ring: producer writes only head, consumer only tail; occupancy is
        // derived from (head - tail), never a shared counter. Cache-line aligned to avoid
        // false sharing between the two cores.
        struct alignas(CACHE_LINE_SIZE) Queue
        {
            alignas(CACHE_LINE_SIZE) volatile uint16_t head;                // Producer writes ONLY (enqueue)
            uint8_t _pad_head[CACHE_LINE_SIZE - sizeof(volatile uint16_t)]; // Padding to 64-byte cache-line

            alignas(CACHE_LINE_SIZE) volatile uint16_t tail;                // Consumer writes ONLY (processQueue)
            uint8_t _pad_tail[CACHE_LINE_SIZE - sizeof(volatile uint16_t)]; // Padding to 64-byte cache-line

            alignas(QUEUE_ENTRY_SIZE) QueueEntry buffer[QUEUE_SIZE]; // 1024 × 32B = 32KB
        };
    #endif

        class PIOI2CWire : public TwoWire // PIO-based I2C Wire implementation, inherits from TwoWire original Wire class
        {
          private:
    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
            // === Single Queue Members ===
            Queue _queue; // Single queue for all transfers

            // === Statistics ===
            volatile uint32_t _transfersCompleted; // Total transfers completed
            volatile uint32_t _queueOverflows;     // Queue overflow events

            // === Extended Performance Metrics ===
            volatile uint16_t _queuePeakCount;        // Peak queue usage (max entries at once)
            volatile uint32_t _totalProcessCalls;     // Total processQueue() calls
            volatile uint32_t _totalEntriesProcessed; // Total entries processed
            volatile uint32_t _dmaTransfers;          // DMA transfers (if available)
            volatile uint32_t _blockingTransfers;     // Blocking transfers (fallback)
            uint32_t _lastStatsUpdate;                // Timestamp of last stats update
    #endif

            // Last completed transfer's TwoWire status (0 ok, 2 addr NACK, 3 data NACK, 4 other).
            // Sync paths return their own code directly; this only surfaces async results via lastAsyncError().
            volatile uint8_t _lastTxStatus = 0;
            // Set by write() when data did not fit the TX buffer -> endTransmission returns 1 (data too long).
            bool _txOverflow = false;

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
            // Set I2C timeout in ms. Pushed down to pio_i2c so wait_idle()/put16() honour it.
            void setTimeout(uint32_t timeout_ms)
            {
                _timeout_ms = timeout_ms;
                if (_pioi2c) _pioi2c->set_timeout_us(timeout_ms * 1000UL);
            }
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

            // 9-clock SDA bus-recovery for a wedged bus. Returns true if SDA was released.
            bool busRecovery();

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

    #ifdef OPENKNX_I2C_USE_ASYNC_QUEUE
            // === Single Queue Methods (Dual-Core Safe) ===

            // Enqueue a transfer (single producer). length 0..MAX_ENTRY_DATA; length==0 is a valid
            // address-only probe. Returns false (and counts an overflow) if full or oversized.
            bool enqueue(uint8_t address, const uint8_t* data, uint16_t length, bool send_stop);

            // Drain the queue - call from the main loop.
            void processQueue();

            // SM-ownership flag serializing reads and the queued-write drain (both drive the same PIO
            // SM/FIFO). Only the flag flip runs with interrupts off; the transfer itself does not, so
            // the watchdog/USB keep running. tryLockSM() never spins: a contended caller backs off.
            volatile bool _smBusy = false; // true while some path owns the PIO SM/FIFO
            inline bool tryLockSM()
            {
                uint32_t s = save_and_disable_interrupts();
                bool acquired = !_smBusy;
                if (acquired) _smBusy = true;
                restore_interrupts(s);
                return acquired;
            }
            inline void unlockSM() { _smBusy = false; }

            // === Queue Status ===
            inline uint16_t queueCount() const
            {
                uint16_t h = _queue.head;
                uint16_t t = _queue.tail;
                return (h >= t) ? (h - t) : (QUEUE_SIZE - t + h);
            }
            inline uint16_t queueFree() const { return QUEUE_SIZE - queueCount() - 1; }

            // Fire-and-forget async status: an async endTransmission returns 0 = "queued", so these
            // let a caller observe the deferred result. lastAsyncError() = last drained transfer's
            // TwoWire code; asyncPending() = entries still queued.
            inline uint8_t lastAsyncError() const { return _lastTxStatus; }
            inline uint16_t asyncPending() const { return queueCount(); }

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
            inline float getQueueEfficiency() const
            {
                // Efficiency: successful transfers / (successful + overflows)
                uint32_t total = _transfersCompleted + _queueOverflows;
                return (total > 0) ? (float)_transfersCompleted / total * 100.0f : 100.0f;
            }
            inline float getAvgEntriesPerCall() const
            {
                // Average entries processed per processQueue() call
                return (_totalProcessCalls > 0) ? (float)_totalEntriesProcessed / _totalProcessCalls : 0.0f;
            }
            inline float getQueueUtilization() const
            {
                // Current queue fill percentage
                return (float)queueCount() / QUEUE_SIZE * 100.0f;
            }
            inline float getPeakUtilization() const
            {
                // Peak queue fill percentage
                return (float)_queuePeakCount / QUEUE_SIZE * 100.0f;
            }
            inline float getDmaRatio() const
            {
                // Percentage of DMA vs blocking transfers
                uint32_t total = _dmaTransfers + _blockingTransfers;
                return (total > 0) ? (float)_dmaTransfers / total * 100.0f : 0.0f;
            }

            // Reset statistics (for benchmarking)
            inline void resetStats()
            {
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
            pio_i2c* _pioi2c;                                 // Underlying PIO I2C instance
            uint32_t _sda, _scl;                              // SDA and SCL pin numbers
            uint32_t _baudrate;                               // I2C clock speed in Hz
            uint32_t _timeout_ms = PIOI2C_DEFAULT_TIMEOUT_MS; // Default timeout 10ms
            uint8_t _txBuffer[PIOI2C_DEFAULT_TX_BUFFER_SIZE]; // Full frame buffer for Display (1024 pixels + overhead)
            size_t _txLen;                                    // Transmit length
            uint8_t _rxBuffer[PIOI2C_DEFAULT_RX_BUFFER_SIZE]; // Important, we need min 256 buffer, especially for i2c Display...
            size_t _rxLen;                                    // Receive length
            size_t _rxPos;                                    // Receive position
            uint8_t _address;                                 // I2C address
        };
    } // namespace I2C
} // namespace OpenKNX
#endif // defined(ARDUINO_ARCH_RP2040)