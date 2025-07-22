// -------------------------------------------------- //
// This file contains the PIO program for WS2812 LEDs //
// -------------------------------------------------- //

#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include "hardware/pio.h"

// PIO assembly program for WS2812
static const uint16_t ws2812_program_instructions[] = {
    0x6221, // OUT X, 1            ; Shift out 1 bit to X
    0x1123, // JMP !X, 3           ; If X is 0, jump to instruction 3
    0x1400, // NOP                 ; Delay for T1H (high time for '1')
    0xA442, // SET PINS, 1         ; Set pin high
    0xA023, // SET PINS, 0         ; Set pin low
    0xA442, // SET PINS, 1         ; Set pin high
    0xA023, // SET PINS, 0         ; Set pin low
};

// Length of the program
#define ws2812_program_length  (sizeof(ws2812_program_instructions) / sizeof(ws2812_program_instructions[0]))

// PIO program metadata
static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length = ws2812_program_length,
    .origin = -1,
};

// Define the ws2812_program_get_default_config function
pio_sm_config ws2812_program_get_default_config(uint offset) {
    // Initialize the state machine configuration
    pio_sm_config c = {
        .clkdiv = 1.0f, // Default clock divider
        .wrap = {
            .wrap_target = offset,
            .wrap = offset + ws2812_program_length - 1
        },
        .out_shift = {
            .shift_right = false,
            .autopull = true,
            .pull_threshold = 24 // 24 bits for RGB
        },
        .fifo = {
            .join = PIO_FIFO_JOIN_TX
        },
        .sideset = {
            .count = 1,
            .optional = false,
            .pindirs = false
        }
    };

    return c;
}

// Initialize the PIO program for WS2812
void ws2812_program_init(PIO pio, uint sm, uint offset, uint pin, uint freq, bool rgbw) {
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_shift(&c, true, false, 24); // Shift out 24 bits (RGB)
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, (float)clock_get_hz(clk_sys) / freq);
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

#endif