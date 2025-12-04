#pragma once
/**
 * @file        pio_i2c.h
 * @brief       PIO I2C Driver for RP2040/RP2350 microcontrollers
 * @version     0.0.1
 * @date        2025-10-30
 * @copyright   Copyright (c) 2025, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0 using the BSD-3-Clause license for
 *              the PIO I2C program code provided by Raspberry Pi.
 */
#if defined(ARDUINO_ARCH_RP2040)

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "i2c.pio.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OPENKNX_PIO_I2C_DMA
#include "hardware/dma.h"
#endif

//#define PIO_I2C_USE_STATIC_PIO
#ifdef PIO_I2C_USE_STATIC_PIO
#define PIO_I2C_USE_PIO_INSTANCE pio0 // We will use PIO0 for I2C
#endif
#define PIO_I2C_TIMEOUT_US 50000      // In microseconds (50ms - Required for PIO I2C)

/*
 * @brief PIO I2C instance structure
 */
struct pio_i2c_inst
{
    uint32_t sda;       // SDA pin number
    uint32_t scl;       // SCL pin number (must be SDA + 1)
    uint32_t baudrate;  // I2C baudrate in Hz
    PIO pio;            // PIO instance (automatically allocated)
    uint sm;            // State machine index (Automatically allocated)
    int prog_offset;    // Program offset in PIO instruction memory
};
typedef struct pio_i2c_inst pio_i2c_inst_t;

class pio_i2c
{
  public:
    pio_i2c(uint32_t sda, uint32_t scl, uint32_t baudrate);                         // Constructor
    ~pio_i2c();                                                                     // Destructor
    void set_baudrate(uint32_t baudrate);                                           // Set I2C baudrate
    int write_blocking(uint8_t addr, const uint8_t* data, size_t len, bool nostop); // Write blocking
    int read_blocking(uint8_t addr, uint8_t* data, size_t len, bool nostop);        // Read blocking

    pio_sm_config i2c_program_get_default_config(unsigned int offset); // Get default PIO SM config
    void reset();                                                      // Reset the PIO I2C instance

    // private:
    PIO get_pio() const;
    uint get_sm() const;
    uint get_offset() const;

    bool check_error(PIO pio, uint sm);                                               // Check for errors
    void put_or_err(PIO pio, uint sm, uint16_t data);                                 // Put data or set error
    void put16(PIO pio, uint sm, uint16_t data);                                      // Put 16 bits
    void rx_enable(PIO pio, uint sm, bool en);                                        // Enable RX
    void resume_after_error(PIO pio, uint sm);                                        // Resume after error
    uint8_t get(PIO pio, uint sm);                                                    // Get data

    void start(PIO pio, uint sm);                                                     // Start condition
    void stop(PIO pio, uint sm);                                                      // Stop condition
    void repstart(PIO pio, uint sm);                                                  // Repeated start condition
    void wait_idle(PIO pio, uint sm);                                                 // Wait until bus is idle
    void i2c_program_init(PIO pio, uint sm, uint offset, uint pin_sda, uint pin_scl); // Initialize PIO I2C program

  public:
    pio_i2c_inst_t* _inst;                                                                                // PIO I2C instance
    int _write_blocking(PIO pio, uint sm, uint8_t addr, uint8_t* txbuf, uint len, bool send_stop = true); // Write blocking
    int _read_blocking(PIO pio, uint sm, uint8_t addr, uint8_t* rxbuf, uint len, bool send_stop = true);  // Read blocking

#ifdef OPENKNX_PIO_I2C_DMA
    // DMA members - public so PIOI2CWire can check status
    int _dma_tx = -1;          // DMA channel for TX (FIFO writes)
    int _dma_rx = -1;          // DMA channel for RX (FIFO reads)
    bool _dma_available = false; // DMA successfully initialized
    bool _last_send_stop = true; // Track last transfer's stop flag for Repeated START
    
    // DMA transfer functions (non-blocking) - public for PIOI2CWire queue processing
    int _write_dma(PIO pio, uint sm, uint8_t addr, uint8_t* txbuf, uint len, bool send_stop = true);
    int _read_dma(PIO pio, uint sm, uint8_t addr, uint8_t* rxbuf, uint len, bool send_stop = true);
    
#ifdef OPENKNX_DEBUG
    // Statistics (only in debug builds)
    uint32_t _dma_write_count = 0;
    uint32_t _dma_read_count = 0;
    uint32_t _blocking_write_count = 0;
    uint32_t _blocking_read_count = 0;
#endif
#endif
};
#endif // !defined(ARDUINO_ARCH_RP2040)
