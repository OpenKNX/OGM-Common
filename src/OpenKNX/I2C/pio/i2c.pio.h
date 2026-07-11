#pragma once
/*
 * This pio i2c program instruction is generated from the i2c.pio file using
 * the pioasm tool provided by the Raspberry Pi Pico SDK.
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Modifications (c) 2026 Erkan Çolak (erkan@colak.de)
 */

#if defined(ARDUINO_ARCH_RP2040)
    #if !PICO_NO_HARDWARE
        #include "hardware/pio.h"
    #endif

    //
    // Set pio i2c program
    //
    #define i2c_wrap_target 12         // Target for wrapping
    #define i2c_wrap 17                // Wrap point
    #define i2c_offset_entry_point 12u // Entry point of the program

static const uint16_t i2c_program_instructions[] = {
    0x008c, //  0: jmp    y--, 12                           - Jump to wrap target
    0xc030, //  1: irq    wait 0 rel                        - Clear interrupt
    0xe027, //  2: set    x, 7                              - Set X to 7 (bit counter)
    0x6781, //  3: out    pindirs, 1             [7]        - Set SDA as output
    0xba42, //  4: nop                    side 1 [2]        - Set SCL high
    0x24a1, //  5: wait   1 pin, 1               [4]        - Wait for SCL high
    0x4701, //  6: in     pins, 1                [7]        - Read SDA
    0x1743, //  7: jmp    x--, 3          side 0 [7]        - Loop for 8 bits
    0x6781, //  8: out    pindirs, 1             [7]        - Set SDA as output
    0xbf42, //  9: nop                    side 1 [7]        - Set SCL high
    0x27a1, // 10: wait   1 pin, 1               [7]        - Wait for SCL high
    0x12c0, // 11: jmp    pin, 0          side 0 [2]        - Check ACK/NACK
            //     .wrap_target
    0x6026, // 12: out    x, 6                             - Set X to 6 (bit counter for STOP)
    0x6041, // 13: out    y, 1                             - Set Y to 1 (STOP sequence)
    0x0022, // 14: jmp    !x, 2                            - Loop for STOP
    0x6060, // 15: out    null, 32                         - Dummy out to generate clock pulses
    0x60f0, // 16: out    exec, 16                         - Execute STOP
    0x0050, // 17: jmp    x--, 16                          - Jump to wrap
            //     .wrap
};

    #define I2C_PROGRAM_LENGTH 18 // number of total program instructions

    #if !PICO_NO_HARDWARE
static const struct pio_program i2c_program = {
    .instructions = i2c_program_instructions, // Instruction table
    .length = I2C_PROGRAM_LENGTH,             // Number of instructions
    .origin = -1,                             // No specific origin
};
    #endif

    //
    // Set scl_sda program
    //
    #define set_scl_sda_wrap_target 0 // Target for wrapping
    #define set_scl_sda_wrap 3        // Wrap point

static const uint16_t set_scl_sda_program_instructions[] = {
    //     .wrap_target
    0xf780, //  0: set    pindirs, 0      side 0 [7]
    0xf781, //  1: set    pindirs, 1      side 0 [7]
    0xff80, //  2: set    pindirs, 0      side 1 [7]
    0xff81, //  3: set    pindirs, 1      side 1 [7]
            //     .wrap
};

    #if !PICO_NO_HARDWARE
static const struct pio_program set_scl_sda_program = {
    .instructions = set_scl_sda_program_instructions,
    .length = 4,
    .origin = -1,
};

// Define order of our instruction table
enum
{
    I2C_SC0_SD0 = 0,
    I2C_SC0_SD1,
    I2C_SC1_SD0,
    I2C_SC1_SD1
};

    #endif // PICO_NO_HARDWARE
#endif     // defined(ARDUINO_ARCH_RP2040)
