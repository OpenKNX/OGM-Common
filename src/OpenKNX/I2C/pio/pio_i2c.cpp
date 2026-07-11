/**
 * @file        pio_i2c.cpp
 * @brief       PIO I2C Driver for RP2040/RP2350 microcontrollers
 * @version     0.0.1
 * @date        2025-10-30
 * @copyright   Copyright (c) 2025, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0 using the BSD-3-Clause license for
 *              the PIO I2C program code provided by Raspberry Pi.
 */
#if defined(ARDUINO_ARCH_RP2040)

    #include "pio_i2c.h"
    #include "hardware/sync.h"
    #include <pico/stdlib.h>

/*
 * @brief Get default PIO SM config for I2C program
 */
pio_sm_config pio_i2c::i2c_program_get_default_config(unsigned int offset)
{
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + i2c_wrap_target, offset + i2c_wrap);
    sm_config_set_sideset(&c, 2, true, true);
    return c;
}

    #ifdef PIO_I2C_USE_STATIC_PIO
/*
 * @brief PIO I2C constructor
 */
pio_i2c::pio_i2c(uint32_t sda, uint32_t scl, uint32_t baudrate)
{
    _inst = new pio_i2c_inst_t{}; // Allocate instance
    if (!_inst) return;           // Allocation failed

    _inst->sda = sda;
    _inst->scl = scl;
    _inst->baudrate = baudrate;

    _inst->pio = PIO_I2C_USE_PIO_INSTANCE;             // Use PIO 0 for I2C
    _inst->sm = pio_claim_unused_sm(_inst->pio, true); // Check for available state machine
    if (_inst->sm < 0)
    {
        delete _inst;
        _inst = nullptr;
        return; // No available state machine
    }

    _inst->prog_offset = pio_add_program(_inst->pio, &i2c_program); // Load program into PIO instruction memory

    if (_inst->prog_offset <= PICO_ERROR_GENERIC) // Check for errors
    {
        pio_sm_unclaim(_inst->pio, _inst->sm);
        delete _inst;
        _inst = nullptr;
        return; // Failed to add program
    }

    // Initialize the PIO I2C program
    i2c_program_init(_inst->pio, _inst->sm, _inst->prog_offset, _inst->sda, _inst->scl);
}

/*
 * @brief PIO I2C destructor
 */
pio_i2c::~pio_i2c()
{
    if (_inst) // Clean up instance
    {
        if (_inst->sm >= 0) // If state machine was claimed
        {
            pio_sm_set_enabled(_inst->pio, _inst->sm, false); // Disable state machine
            pio_sm_unclaim(_inst->pio, _inst->sm);            // Unclaim state machine
        }
        pio_remove_program(_inst->pio, &i2c_program, _inst->prog_offset); // Remove program from PIO
        delete _inst;                                                     // Delete instance
        _inst = nullptr;                                                  // Clear pointer
    }
}
    #else // PIO_I2C_USE_STATIC_PIO
/*
 * @brief PIO I2C constructor
 */
pio_i2c::pio_i2c(uint32_t sda, uint32_t scl, uint32_t baudrate)
{
    _inst = new pio_i2c_inst_t{}; // Allocate instance
    if (!_inst) return;           // Allocation failed

    _inst->sda = sda;           // Set SDA pin
    _inst->scl = scl;           // Set SCL pin
    _inst->baudrate = baudrate; // Set baudrate

        #if PICO_PIO_USE_GPIO_BASE // Use GPIO base support, available on RP2350B and later
    // On RP2350B and later: Use GPIO base support for flexible pin assignment, especially for higher GPIO numbers
    PIO pio_temp = nullptr; // Temporary PIO instance
    uint sm_temp = 0;       // Temporary state machine index
    uint offset_temp = 0;   // Temporary program offset

    uint32_t gpio_base = sda;                                      // Base GPIO pin (SDA)
    uint32_t gpio_count = (scl > sda ? scl : sda) - gpio_base + 1; // Number of GPIOs needed

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
        &i2c_program, // Use SDK to claim SM and add program
        &pio_temp,    // Return PIO instance
        &sm_temp,     // Return state machine index
        &offset_temp, // Return program offset
        gpio_base,    // Base GPIO pin
        gpio_count,   // Number of GPIOs needed
        true          // Allow setting GPIO base if needed
    );

    if (!success) // Check for success
    {
        delete _inst;    // Clean up instance
        _inst = nullptr; // Clear pointer
        return;          // Failed to claim SM and add program
    }

    _inst->pio = pio_temp;            // Set PIO instance
    _inst->sm = sm_temp;              // Set state machine index
    _inst->prog_offset = offset_temp; // Set program offset

        #else
    // On RP2040 and earlier: Use standard method to claim SM and add program
    PIO pio_temp = nullptr; // Temporary PIO instance
    uint sm_temp = 0;       // Temporary state machine index
    uint offset_temp = 0;   // Temporary program offset

    bool success = pio_claim_free_sm_and_add_program(
        &i2c_program, // Use SDK to claim SM and add program
        &pio_temp,    // Return PIO instance
        &sm_temp,     // Return state machine index
        &offset_temp  // Return program offset
    );

    if (!success) // Check for success
    {
        delete _inst;    // Clean up instance
        _inst = nullptr; // Clear pointer
        return;          // Failed to claim SM and add program
    }

    _inst->pio = pio_temp;            // Set PIO instance
    _inst->sm = sm_temp;              // Set state machine index
    _inst->prog_offset = offset_temp; // Set program offset
        #endif
    // Initialize the PIO I2C program
    i2c_program_init(_inst->pio, _inst->sm, _inst->prog_offset, _inst->sda, _inst->scl);

        #ifdef OPENKNX_PIO_I2C_DMA
    // Try to claim DMA channels for non-blocking I2C
    _dma_tx = dma_claim_unused_channel(false); // TX channel (don't panic if unavailable)
    _dma_rx = dma_claim_unused_channel(false); // RX channel (don't panic if unavailable)

    if (_dma_tx >= 0 && _dma_rx >= 0)
    {
        _dma_available = true;
    }
    else
    {
        // Fallback: Release any claimed channel and use blocking mode
        if (_dma_tx >= 0) dma_channel_unclaim(_dma_tx);
        if (_dma_rx >= 0) dma_channel_unclaim(_dma_rx);
        _dma_tx = -1;
        _dma_rx = -1;
        _dma_available = false;
        // No log here - happens in PIOI2CWire.cpp begin()
    }
        #endif
}

/*
 * @brief PIO I2C destructor
 */
pio_i2c::~pio_i2c()
{
        #ifdef OPENKNX_PIO_I2C_DMA
    // Release DMA channels if claimed
    if (_dma_tx >= 0) dma_channel_unclaim(_dma_tx);
    if (_dma_rx >= 0) dma_channel_unclaim(_dma_rx);
        #endif

    if (_inst) // Clean up instance
    {
        if (_inst->sm >= 0) // If state machine was claimed
        {
            pio_sm_set_enabled(_inst->pio, _inst->sm, false);
            pio_remove_program_and_unclaim_sm(
                &i2c_program,      // Remove program and unclaim SM
                _inst->pio,        // PIO instance
                _inst->sm,         // State machine index
                _inst->prog_offset // Program offset
            );
        }
        delete _inst;
        _inst = nullptr;
    }
}
    #endif // PIO_I2C_USE_STATIC_PIO

/*
 * @brief Set I2C baudrate
 * SAFETY: NULL-check prevents crash if constructor failed
 */
void pio_i2c::set_baudrate(uint32_t baudrate)
{
    if (!_inst) return; // SAFE: Constructor failed, nothing to do
    _inst->baudrate = baudrate;

    float div = (float)clock_get_hz(clk_sys) / (32 * baudrate); // Calculate clock divider
    pio_sm_set_clkdiv(_inst->pio, _inst->sm, div);              // Set clock divider

    pio_sm_clkdiv_restart(_inst->pio, _inst->sm); // Restart clock divider
}

/*
 * @brief Reset the PIO I2C instance
 * SAFETY: NULL-check prevents crash if constructor failed
 */
void pio_i2c::reset()
{
    if (!_inst) return; // SAFE: Constructor failed, nothing to do

    PIO pio = _inst->pio;
    uint sm = _inst->sm;

    // Stop and cleanup the state machine
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);
    pio_interrupt_clear(pio, sm);

    // Re-initialize the state machine
    pio_sm_restart(pio, sm);
    pio_sm_clkdiv_restart(pio, sm);

    // Jump to the program entry point
    pio_sm_exec(pio, sm, pio_encode_jmp(_inst->prog_offset + i2c_offset_entry_point));

    // Set SDA and SCL pins to high, which is idle state for I2C
    // uint32_t both_pins = (1u << _inst->sda) | (1u << _inst->scl);
    // pio_sm_set_pins_with_mask(pio, sm, both_pins, both_pins);

    // Enable the state machine
    pio_sm_set_enabled(pio, sm, true);
}

/*
 * @brief Check for errors in the PIO I2C instance
 */
bool pio_i2c::check_error(PIO pio, uint sm)
{
    return pio_interrupt_get(pio, sm);
}

/*
 * @brief Classic 9-clock SDA bus-recovery for a wedged/stuck bus.
 *
 * A slave interrupted mid-byte can hold SDA low; bit-bang up to 9 SCL pulses to walk it
 * through the byte + ACK so it releases SDA, then issue a manual STOP. Temporarily hands
 * the pins to SIO, then restores PIO control. Only call when the SM is idle.
 *
 * @return true if SDA is released afterwards, false if still stuck.
 */
bool pio_i2c::bus_recovery()
{
    if (!_inst) return false; // SAFE: constructor failed

    const uint sda = _inst->sda;
    const uint scl = _inst->scl;

    // Half-period for the recovery clock (~100 kHz regardless of configured speed)
    const uint32_t half_us = 5;

    // Hand the pins to SIO so we can bit-bang open-drain.
    gpio_set_function(sda, GPIO_FUNC_SIO);
    gpio_set_function(scl, GPIO_FUNC_SIO);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    // Open-drain: never drive high. High == input (let pull-up win), low == output-low.
    gpio_put(sda, 0);
    gpio_put(scl, 0);
    gpio_set_dir(sda, GPIO_IN); // release SDA
    gpio_set_dir(scl, GPIO_IN); // release SCL

    // Up to 9 SCL pulses while SDA stays low.
    for (int i = 0; i < 9 && gpio_get(sda) == 0; i++)
    {
        gpio_set_dir(scl, GPIO_OUT); // drive SCL low
        busy_wait_us(half_us);
        gpio_set_dir(scl, GPIO_IN); // release SCL high
        busy_wait_us(half_us);
    }

    // Manual STOP: SDA low -> SCL high -> SDA high (rising SDA while SCL high).
    gpio_set_dir(sda, GPIO_OUT); // SDA low
    busy_wait_us(half_us);
    gpio_set_dir(scl, GPIO_IN); // SCL high
    busy_wait_us(half_us);
    gpio_set_dir(sda, GPIO_IN); // SDA high -> STOP
    busy_wait_us(half_us);

    const bool released = (gpio_get(sda) != 0);

    // Give the pins back to the PIO (open-drain via OEOVER=INVERT) and re-arm the SM.
    pio_gpio_init(_inst->pio, sda);
    gpio_set_oeover(sda, GPIO_OVERRIDE_INVERT);
    pio_gpio_init(_inst->pio, scl);
    gpio_set_oeover(scl, GPIO_OVERRIDE_INVERT);
    reset();

    return released;
}

/*
 * @brief Resume operation after an error in the PIO I2C instance
 */
void pio_i2c::resume_after_error(PIO pio, uint sm)
{
    // Interrupts off during recovery to prevent concurrent access.
    uint32_t irq_state = save_and_disable_interrupts();

    pio_sm_clear_fifos(pio, sm);
    pio_interrupt_clear(pio, sm);
    pio_sm_exec(pio, sm, pio_encode_jmp(_inst->prog_offset + i2c_offset_entry_point)); // Jump back to entry point

    restore_interrupts(irq_state);
}

/*
 * @brief Enable/disable RX autopush for the PIO I2C state machine
 */
void pio_i2c::rx_enable(PIO pio, uint sm, bool en)
{
    // Toggle AUTOPUSH without stopping the SM (allowed while running; stopping it crashes
    // concurrent transfers).
    if (en)
        hw_set_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
    else
        hw_clear_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
}

/*
 * @brief Put data into the PIO I2C TX FIFO with a bounded wait.
 * Bounded by _timeout_us and bails out on a bus error so a wedged FIFO can't hang the CPU.
 * @return true if the word was queued, false on timeout (treat as bus error).
 */
bool pio_i2c::put16(PIO pio, uint sm, uint16_t data)
{
    uint32_t start = time_us_32();
    while (pio_sm_is_tx_fifo_full(pio, sm))
    {
        if (check_error(pio, sm)) return false;                 // NACK/error: stop feeding
        if ((time_us_32() - start) > _timeout_us) return false; // Wedged FIFO (honours setTimeout)
        tight_loop_contents();
    }
    *(volatile io_rw_16*)&pio->txf[sm] = data;
    return true;
}

/*
 * @brief Put data into the PIO I2C TX FIFO (blocking)
 */
static void pio_i2c_put16_nocheck(PIO pio, uint sm, uint16_t data)
{
    while (pio_sm_is_tx_fifo_full(pio, sm))
    {
        tight_loop_contents();
    }
    *(volatile io_rw_16*)&pio->txf[sm] = data;
}

/*
 * @brief Get data from the PIO I2C RX FIFO
 */
uint8_t pio_i2c::get(PIO pio, uint sm)
{
    return (uint8_t)pio_sm_get(pio, sm);
}

/*
 * @brief Initialize the PIO I2C program
 */
void pio_i2c::start(PIO pio, uint sm)
{
    pio_i2c_put16_nocheck(pio, sm, 3u << 10);                                      // 4 Instructions
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD1]); // Idle
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD0]); // START
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD0]); // Hold
    pio_i2c_put16_nocheck(pio, sm, pio_encode_mov(pio_isr, pio_null));             // Clear ISR
}

/*
 * @brief Stop the PIO I2C program
 */
void pio_i2c::stop(PIO pio, uint sm)
{
    pio_i2c_put16_nocheck(pio, sm, 2u << 10);                                      // 3 Instructions
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD0]); // Hold
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD0]); // STOP
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD1]); // STOP
}

/*
 * @brief Repeated start condition for the PIO I2C program
 */
void pio_i2c::repstart(PIO pio, uint sm)
{
    // put16_nocheck blocks on a full FIFO, giving automatic flow control vs a still-running
    // DMA without an explicit wait_idle().
    pio_i2c_put16_nocheck(pio, sm, 3u << 10);
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD1]); // Release SDA
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD1]); // Release SCL
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC1_SD0]); // Hold
    pio_i2c_put16_nocheck(pio, sm, set_scl_sda_program_instructions[I2C_SC0_SD0]); // Hold
}

/*
 * @brief Wait until the I2C bus is idle
 */
void pio_i2c::wait_idle(PIO pio, uint sm)
{
    pio->fdebug = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);

    uint32_t start = time_us_32();
    uint32_t timeout = _timeout_us; // ROBUSTNESS: honour the configured setTimeout budget

    while (!(pio->fdebug & (1u << (PIO_FDEBUG_TXSTALL_LSB + sm))))
    {
        if (check_error(pio, sm))
        {
            break; // Error occurred, immediately exit
        }

        if ((time_us_32() - start) > timeout) // Timeout check
        {
            pio_sm_drain_tx_fifo(pio, sm); // Force cleanup on timeout
            break;
        }
        tight_loop_contents(); // Prevent busy-waiting
    }
}

/*
 * @brief Write data to the I2C bus
 */
int pio_i2c::_write_blocking(PIO pio, uint sm, uint8_t addr, uint8_t* txbuf, uint len, bool send_stop)
{
    int err = 0;

    pio_interrupt_clear(pio, sm); // Clear any previous error

    // Repeated START: Wenn letzter Transfer kein STOP hatte, RESTART senden
    #ifdef OPENKNX_PIO_I2C_DMA
    if (!_last_send_stop)
    {
        repstart(pio, sm); // RESTART condition
    }
    else
    {
        start(pio, sm); // START condition
    }
    #else
    start(pio, sm); // Immer START (kein State-Tracking ohne DMA)
    #endif
    rx_enable(pio, sm, false); // Disable RX

    // Count data bytes clocked out to distinguish address NACK (none sent) from data NACK.
    uint dataSent = 0;

    uint8_t addr_byte = (addr << 1) | 0;
    put16(pio, sm, (addr_byte << 1) | 1u); // Shift to bits 8:1, add NAK bit

    while (len && !check_error(pio, sm)) // Send data bytes
    {
        if (!pio_sm_is_tx_fifo_full(pio, sm)) // Check if TX FIFO is not full
        {
            --len;
            // Data byte: Instr=0, Final=(last byte), Data, NAK=1
            put16(pio, sm, (*txbuf++ << 1) | ((len == 0) << 9) | 1u);
            ++dataSent;
        }
    }

    if (send_stop)
    {
        stop(pio, sm); // STOP impliziert IDLE
    }
    else
    {
        wait_idle(pio, sm); // Ohne STOP: warten für Repeated START
    }

    if (check_error(pio, sm)) // Check for errors
    {
        // No data byte accepted yet => address NACK, else data NACK.
        err = (dataSent == 0) ? PIO_I2C_ERR_ADDR_NACK : PIO_I2C_ERR_DATA_NACK;
        resume_after_error(pio, sm); // Resume operation after error

        // Do we need a extra Cleanup after error. Using resume_after_error should be enough
        // stop(pio, sm);
        // wait_idle(pio, sm);
        // pio_interrupt_clear(pio, sm);

    #ifdef OPENKNX_PIO_I2C_DMA
        // Bei Error: State zurücksetzen! Nächster Transfer braucht START, nicht RESTART
        _last_send_stop = true;
    #endif
    }
    #ifdef OPENKNX_PIO_I2C_DMA
    else
    {
        // Nur bei Success: State für nächsten Transfer merken (Repeated START Logic)
        _last_send_stop = send_stop;
    }
    #endif

    return err;
}

/*
 * @brief Read data from the I2C bus
 */
int pio_i2c::_read_blocking(PIO pio, uint sm, uint8_t addr, uint8_t* rxbuf, uint len, bool send_stop)
{
    int err = 0;
    pio_interrupt_clear(pio, sm); // Clear any previous error

    // Repeated START: Wenn letzter Transfer kein STOP hatte, RESTART senden
    #ifdef OPENKNX_PIO_I2C_DMA
    if (!_last_send_stop)
    {
        repstart(pio, sm); // RESTART condition
    }
    else
    {
        start(pio, sm); // START condition
    }
    #else
    start(pio, sm); // Immer START (kein State-Tracking ohne DMA)
    #endif
    rx_enable(pio, sm, true); // Enable RX

    while (!pio_sm_is_rx_fifo_empty(pio, sm)) // Flush RX FIFO
    {
        (void)get(pio, sm); // Discard data
    }

    uint8_t addr_byte = (addr << 1) | 1;   // Read address
    put16(pio, sm, (addr_byte << 1) | 1u); // Shift to bits 8:1

    uint32_t tx_remain = len;
    bool first = true;

    while ((tx_remain || len) && !check_error(pio, sm)) // Read data bytes
    {
        if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm)) // Check if TX FIFO is not full
        {
            --tx_remain;
            // Send 0xFF (release SDA for read), with NAK on last byte
            put16(pio, sm, (0xffu << 1) | (tx_remain ? 0 : (1u << 9) | (1u << 0)));
        }
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) // Check if RX FIFO has data
        {
            if (first) // First byte is address ACK, discard it
            {
                (void)get(pio, sm);
                first = false;
            }
            else
            {
                --len;
                *rxbuf++ = get(pio, sm); // Read data byte
            }
        }
    }

    if (send_stop)
    {
        stop(pio, sm); // Send STOP condition
    }

    // Address ACK never received (first still true) => address NACK.
    const int readErr = first ? PIO_I2C_ERR_ADDR_NACK : PIO_I2C_ERR_OTHER;

    if (check_error(pio, sm))
    {
        err = readErr;
        resume_after_error(pio, sm); // Resume operation after error

    #ifdef OPENKNX_PIO_I2C_DMA
        // Bei Error: State zurücksetzen! Nächster Transfer braucht START
        _last_send_stop = true;
    #endif
    }
    else
    {
        wait_idle(pio, sm); // Ensure bus is idle

        if (check_error(pio, sm)) // Check for errors again
        {
            err = readErr;
            resume_after_error(pio, sm); // Resume operation after error

    #ifdef OPENKNX_PIO_I2C_DMA
            // Bei Error: State zurücksetzen!
            _last_send_stop = true;
    #endif
        }
    #ifdef OPENKNX_PIO_I2C_DMA
        else
        {
            // Nur bei Success: State für nächsten Transfer setzen
            _last_send_stop = send_stop;
        }
    #endif
    }

    return err;
}

    #ifdef OPENKNX_PIO_I2C_DMA
/*
 * @brief DMA-based write (non-blocking)
 */
int pio_i2c::_write_dma(PIO pio, uint sm, uint8_t addr, uint8_t* txbuf, uint len, bool send_stop)
{
    if (!_dma_available)
    {
        return _write_blocking(pio, sm, addr, txbuf, len, send_stop);
    }

    if (dma_channel_is_busy(_dma_tx))
    {
        return PIO_I2C_BUSY; // Busy (distinct from a NACK)
    }

        #ifdef OPENKNX_DEBUG
    _dma_write_count++; // Count DMA transfers
        #endif

    // Clamp to the DMA buffer size (28); should not happen with correct queue sizing.
    if (len > 28)
    {
        len = 28;
    }

    int err = 0;

    // start/repstart clear the interrupt internally, so no pio_interrupt_clear() here.
    // Repeated START if the last transfer had no STOP.
    if (!_last_send_stop)
    {
        repstart(pio, sm); // RESTART condition
    }
    else
    {
        start(pio, sm); // START condition
    }
    rx_enable(pio, sm, false);

    uint8_t addr_byte = (addr << 1) | 0;
    put16(pio, sm, (addr_byte << 1) | 1u); // Shift to bits 8:1, add NAK bit

    // Prepare all data into the instance buffer (a static one would corrupt).
    for (size_t i = 0; i < len; i++) // len already clamped to 28
    {
        _dma_buffer[i] = (txbuf[i] << 1) | ((i == len - 1) << 9) | 1u;
    }

    // DMA config: 16-bit, high priority, DREQ-paced to PIO TX-FIFO space.
    dma_channel_config c = dma_channel_get_default_config(_dma_tx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    channel_config_set_high_priority(&c, true);

    // Configure without starting (avoids a race), then start.
    dma_channel_configure(
        _dma_tx,
        &c,
        &pio->txf[sm],
        _dma_buffer,
        len,
        false);

    dma_channel_start(_dma_tx);

    // Only wait when send_stop (protocol STOP); otherwise return immediately and let the next
    // repstart() sync on the FIFO. PIO FIFO + DREQ throttling gives automatic flow control.
    if (send_stop)
    {
        dma_channel_wait_for_finish_blocking(_dma_tx);
        wait_idle(pio, sm); // Ensure bus idle before STOP
        stop(pio, sm);

        if (pio->irq & (1u << sm))
        {
            err = PIO_I2C_ERR_OTHER; // DMA burst can't attribute addr vs data NACK
            resume_after_error(pio, sm);
        }
        _last_send_stop = true;
    }
    else
    {
        // Mid-batch: fire-and-forget; the next repstart() blocks on the FIFO if PIO is still busy.
        _last_send_stop = false;
    }

    return err;
}

/*
 * @brief DMA-based read (non-blocking, timer-interrupt safe)
 */
int pio_i2c::_read_dma(PIO pio, uint sm, uint8_t addr, uint8_t* rxbuf, uint len, bool send_stop)
{
    if (!_dma_available)
    {
        // Fallback to blocking mode
        return _read_blocking(pio, sm, addr, rxbuf, len, send_stop);
    }

    // Check if previous DMA transfer still busy
    if (dma_channel_is_busy(_dma_tx))
    {
        return PIO_I2C_BUSY; // Busy, caller should retry
    }

    // Simultaneous TX+RX DMA can conflict, so bail if either channel is busy.
    if (dma_channel_is_busy(_dma_rx))
    {
        return PIO_I2C_BUSY; // RX busy, wait for it to complete
    }
    if (dma_channel_is_busy(_dma_tx))
    {
        return PIO_I2C_BUSY; // TX busy, wait for it to complete
    }

        #ifdef OPENKNX_DEBUG
    _dma_read_count++; // Count DMA transfers
        #endif

    int err = 0;

    // start/repstart clear the interrupt internally. Repeated START if the last had no STOP.
    if (!_last_send_stop)
    {
        repstart(pio, sm); // RESTART condition
    }
    else
    {
        start(pio, sm); // START condition
    }
    rx_enable(pio, sm, true);

    // Flush RX FIFO
    while (!pio_sm_is_rx_fifo_empty(pio, sm))
    {
        (void)get(pio, sm);
    }

    uint8_t addr_byte = (addr << 1) | 1;   // Read address
    put16(pio, sm, (addr_byte << 1) | 1u); // Shift to bits 8:1

    // Send read commands
    for (uint i = 0; i < len; i++)
    {
        put16(pio, sm, (0xffu << 1) | (i == len - 1 ? (1u << 9) | 1u : 0));
    }

    // Wait for PIO to start clocking before DMA, else DMA reads an empty RX FIFO (hardware fault).
    while (pio_sm_is_tx_fifo_empty(pio, sm))
    {
        tight_loop_contents();
    }

    // Configure without starting (avoids a race).
    dma_channel_config c = dma_channel_get_default_config(_dma_rx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, false);              // Fixed read (FIFO)
    channel_config_set_write_increment(&c, true);              // Increment write
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false)); // RX DREQ

    dma_channel_configure(
        _dma_rx,
        &c,
        rxbuf,         // Write to buffer
        &pio->rxf[sm], // Read from RX FIFO
        len + 1,       // +1 for address ACK
        false);

    dma_channel_start(_dma_rx);

    // Read must complete before STOP.
    dma_channel_wait_for_finish_blocking(_dma_rx);

    if (send_stop)
    {
        stop(pio, sm);
    }

    wait_idle(pio, sm); // Wait for bus idle (needed for Repeated START)

    // Discard first byte (address ACK); only when len > 0 to avoid reading rxbuf[1].
    if (len > 0)
    {
        memmove(rxbuf, rxbuf + 1, len);
    }

    if (pio->irq & (1u << sm))
    {
        err = PIO_I2C_ERR_OTHER; // DMA read: report generic bus error
        resume_after_error(pio, sm);
        // Bei Error: State zurücksetzen!
        _last_send_stop = true;
    }
    else
    {
        // Success: State für nächsten Transfer setzen
        _last_send_stop = send_stop;
    }

    return err;
}
    #endif // OPENKNX_PIO_I2C_DMA

/*
 * @brief Public write blocking method (ALWAYS uses blocking implementation)
 * SAFETY: NULL-checks prevent crash if constructor failed
 */
int pio_i2c::write_blocking(uint8_t addr, const uint8_t* data, size_t len, bool nostop)
{
    if (!_inst) return -1;           // SAFE: Constructor failed
    if (!data && len > 0) return -1; // SAFE: Invalid parameters
    #ifdef OPENKNX_DEBUG
        #ifdef OPENKNX_PIO_I2C_DMA
    _blocking_write_count++; // Count blocking transfers
        #endif
    #endif
    return _write_blocking(_inst->pio, _inst->sm, addr, (uint8_t*)data, len, !nostop);
}

/*
 * @brief Public read blocking method
 * SAFETY: NULL-checks prevent crash if constructor failed
 */
int pio_i2c::read_blocking(uint8_t addr, uint8_t* data, size_t len, bool nostop)
{
    if (!_inst) return -1;           // SAFE: Constructor failed
    if (!data && len > 0) return -1; // SAFE: Invalid parameters
    #ifdef OPENKNX_PIO_I2C_DMA
    if (_dma_available)
    {
        #ifdef OPENKNX_DEBUG
        _dma_read_count++; // Count DMA transfers
        #endif
        return _read_dma(_inst->pio, _inst->sm, addr, data, len, !nostop);
    }
        #ifdef OPENKNX_DEBUG
    _blocking_read_count++; // Count blocking transfers
        #endif
    #endif
    return _read_blocking(_inst->pio, _inst->sm, addr, data, len, !nostop);
}

/*
 * @brief Get the PIO instance being used
 * SAFETY: Returns nullptr if constructor failed
 */
PIO pio_i2c::get_pio() const
{
    return _inst ? _inst->pio : nullptr; // SAFE: Already has NULL-check
}

/*
 * @brief Get the state machine being used
 * SAFETY: Returns 0 if constructor failed
 */
uint pio_i2c::get_sm() const
{
    return _inst ? _inst->sm : 0; // SAFE: Already has NULL-check
}

/*
 * @brief Get the program offset
 * SAFETY: Returns 0 if constructor failed
 */
uint pio_i2c::get_offset() const
{
    return _inst ? _inst->prog_offset : 0; // SAFE: Already has NULL-check
}

/*
 * @brief Initialize the PIO I2C program
 */
void pio_i2c::i2c_program_init(PIO pio, uint sm, uint offset, uint pin_sda, uint pin_scl)
{
    assert(pin_scl == pin_sda + 1);                           // Ensure SCL is SDA + 1. assertion for safety
    pio_sm_config c = i2c_program_get_default_config(offset); // Get default config

    sm_config_set_out_pins(&c, pin_sda, 1);       // Set SDA as output pin
    sm_config_set_set_pins(&c, pin_sda, 1);       // Set SDA as set pin
    sm_config_set_in_pins(&c, pin_sda);           // Set SDA as input pin
    sm_config_set_sideset_pins(&c, pin_scl);      // Set SCL as sideset pin
    sm_config_set_jmp_pin(&c, pin_sda);           // Set JMP pin to SDA
    sm_config_set_out_shift(&c, false, true, 16); // MSB first, autopull
    sm_config_set_in_shift(&c, false, true, 8);   // MSB first, autopush

    float div = (float)clock_get_hz(clk_sys) / (32 * _inst->baudrate); // Calculate clock divider
    sm_config_set_clkdiv(&c, div);                                     // Set clock divider

    // Initialize GPIOs
    gpio_pull_up(pin_scl); // Enable pull-up on SCL
    gpio_pull_up(pin_sda); // Enable pull-up on SDA

    // Configure pins and set to idle state (both high)
    uint32_t both_pins = (1u << pin_sda) | (1u << pin_scl);      // Both pins mask
    pio_sm_set_pins_with_mask(pio, sm, both_pins, both_pins);    // Set both pins high
    pio_sm_set_pindirs_with_mask(pio, sm, both_pins, both_pins); // Set both pins as outputs

    // Configure pins for open-drain operation
    pio_gpio_init(pio, pin_sda);                    // Initialize SDA pin
    gpio_set_oeover(pin_sda, GPIO_OVERRIDE_INVERT); // Set SDA to open-drain
    pio_gpio_init(pio, pin_scl);                    // Initialize SCL pin
    gpio_set_oeover(pin_scl, GPIO_OVERRIDE_INVERT); // Set SCL to open-drain

    // Set both pins low initially
    pio_sm_set_pins_with_mask(pio, sm, 0, both_pins); // Set both pins low initially

    // Disable interrupts for the state machine
    pio_set_irq0_source_enabled(pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + sm), false); // Disable IRQ0s
    pio_set_irq1_source_enabled(pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + sm), false); // Disable IRQ1s
    pio_interrupt_clear(pio, sm);                                                                    // Clear any pending interrupts

    // Initialize and enable the state machine
    pio_sm_init(pio, sm, offset + i2c_offset_entry_point, &c); // Initialize state machine
    pio_sm_set_enabled(pio, sm, true);                         // Enable state machine
}
#endif // defined(ARDUINO_ARCH_RP2040)