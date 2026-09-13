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

// Only the cores' own defines: they gate the PSRAM heap init. Any extra alias could
// enable the macros without an initialized heap.
#if (defined(BOARD_HAS_PSRAM) || defined(RP2350_PSRAM_CS)) && !defined(OPENKNX_DISABLE_PSRAM)
    #define OPENKNX_PSRAM
#endif

// Heap functions differ per core; free()/realloc() detect the region themselves.
#ifdef OPENKNX_PSRAM
    #ifdef ARDUINO_ARCH_RP2040
        #define PSRAM_MALLOC pmalloc
        #define PSRAM_CALLOC pcalloc
        #define PSRAM_REALLOC realloc
    #else
        #define PSRAM_MALLOC ps_malloc
        #define PSRAM_CALLOC ps_calloc
        #define PSRAM_REALLOC ps_realloc
    #endif
    #define psram_new(X) new (PSRAM_MALLOC(sizeof(X))) X
#else
    #define PSRAM_MALLOC malloc
    #define PSRAM_CALLOC calloc
    #define PSRAM_REALLOC realloc
    #define psram_new(X) new X
#endif

// Section name must match the core's linker script, or the data silently ends up in
// normal RAM. NOLOAD: no static init, not zeroed at startup.
#ifdef OPENKNX_PSRAM
    #ifdef ARDUINO_ARCH_RP2040
        #define PSRAM_DATA __attribute__((section(".psram")))
    #else
        // On ESP32, PSRAM_DATA only takes effect if the sdkconfig has
        // CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY=y; otherwise EXT_RAM_BSS_ATTR
        // is an IDF no-op and the variable silently stays in normal RAM (build still
        // succeeds). PSRAM_MALLOC/PsramAllocator are unaffected by this setting.
        #define PSRAM_DATA EXT_RAM_BSS_ATTR
    #endif
#else
    #define PSRAM_DATA
#endif

// RP2350 only — ESP32 cannot execute from PSRAM.
#if defined(OPENKNX_PSRAM) && defined(ARDUINO_ARCH_RP2040)
    #define PSRAM_CODE __attribute__((section(".psram_code")))
#else
    #define PSRAM_CODE
#endif

#ifdef OPENKNX_PSRAM

// psram_delete — ruft Destruktor, dann free() (Gegenstück zu psram_new)
template <typename T>
inline void psram_delete(T* p)
{
    if (p)
    {
        p->~T();
        free(p);
    }
}

// PsramAllocator — Standard-Allocator für STL-Container mit PSRAM
template <typename T>
struct PsramAllocator
{
    using value_type = T;
    T* allocate(size_t n) { return static_cast<T*>(PSRAM_MALLOC(n * sizeof(T))); }
    void deallocate(T* p, size_t) { free(p); }
};

#else

template <typename T>
inline void psram_delete(T* p) { delete p; }

template <typename T>
using PsramAllocator = std::allocator<T>;

#endif
