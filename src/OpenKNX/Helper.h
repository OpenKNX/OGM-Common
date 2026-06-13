#pragma once

#include <Arduino.h>
#include <cstdint>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include "knx.h"

#define delayCheckMillis(last, duration) (millis() - last >= duration)
#define delayCheckMicros(last, duration) (micros() - last >= duration)
#define delayCheck(last, duration) delayCheckMillis(last, duration)
#define delayTimerInit() (max(millis(), 1UL))

#define NO_NUM -987654321.0F // normal NAN-Handling does not work
#define isNum(value) ((value + 10.0) > NO_NUM)

/*
 * Uptime
 * must be called regularly so that the rollovers can be determined
 */
uint32_t uptime(bool result = true);

/*
 * Free Memory
 */
int freeMemory();

/*
 * Write DPT 16 KO Helper
 */
#if MASK_VERSION != 0x091A
void writeDpt16Ko(GroupObject &ko, const char* message, va_list& values);
void writeDpt16Ko(GroupObject &ko, const char* message, ...);
#endif

/*
 * Nuker
 */
#ifdef ARDUINO_ARCH_RP2040
    #include "hardware/flash.h"
    #include "hardware/sync.h"
    #include "pico/sync.h"

/*
 * Erase flash
 */
bool __no_inline_not_in_flash_func(__nukeFlash)(uint32_t offset, size_t count);

#ifdef SERIAL_DEBUG
void printFreeStackSize();
#endif
#endif

/*
 * PSRAM allocation helper macros
 */

// Normalisiere RP2350 PSRAM auf BOARD_HAS_PSRAM
#if defined(PICO_RP2350_PSRAM_CS)
    #define BOARD_HAS_PSRAM 1
#endif

// Dynamische Allocation
#if defined(BOARD_HAS_PSRAM)
    #define HS_MALLOC ps_malloc
    #define HS_CALLOC ps_calloc
    #define HS_REALLOC ps_realloc
    #define ps_new(X) new (ps_malloc(sizeof(X))) X
#else
    #define HS_MALLOC malloc
    #define HS_CALLOC calloc
    #define HS_REALLOC realloc
    #define ps_new(X) new X
#endif

// Statische Daten/Funktionen in PSRAM
#if defined(BOARD_HAS_PSRAM)
    #define PSRAM_DATA __attribute__((section(".psram_data")))
    #define PSRAM_CODE __attribute__((section(".psram_code")))
#else
    #define PSRAM_DATA
    #define PSRAM_CODE
#endif
