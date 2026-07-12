#pragma once
/**
 * @file        HeapCheck.h
 * @brief       Build-flag-guarded boot-time heap-integrity tripwire (ESP32, zero cost when off)
 * @version     0.0.1
 * @date        2026-07-12
 * @copyright   Copyright (c) 2026, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 *
 * A heap-corrupting WRITE past a heap block (e.g. an oversized index into a small allocation during the
 * boot config restore) surfaces far away, later, in Console::showMemory's heap walk
 * (heap_caps_get_largest_free_block). To bisect WHERE it happens, drop OPENKNX_HEAPCHK() markers between
 * the boot restore steps: heap_caps_check_integrity_all() walks the whole heap and validates every block
 * WITHOUT crashing; on the first corruption it returns false (and the ESP heap component logs the
 * offending block+backtrace). The FIRST marker that prints "HEAP CORRUPT" bounds the culprit — the
 * overrun ran between the previous OK marker and this one.
 *
 * Enable for a diagnostic build only: build_flags = ... -D OPENKNX_HEAP_INTEGRITY_TRACE (ESP32 only).
 * Leave it OFF for release builds. See the OGM-Common README (Debug & Development).
 **/

#if defined(OPENKNX_HEAP_INTEGRITY_TRACE) && defined(ARDUINO_ARCH_ESP32)
    #include "esp_heap_caps.h"

    // marker: a short const char* naming the boot step that JUST completed. openknx.logger must exist
    // at the call site (it does in Common.cpp / Flash/Default.cpp). get_largest_free_block is only
    // called on the OK path (the walk just proved the heap intact), never on a corrupt heap.
    #define OPENKNX_HEAPCHK(marker)                                                                    \
        do                                                                                             \
        {                                                                                              \
            if (heap_caps_check_integrity_all(true))                                                   \
                openknx.logger.logWithPrefixAndValues("HEAPCHK", "%s OK free=%u largest=%u", (marker), \
                                                       (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),        \
                                                       (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)); \
            else                                                                                       \
                openknx.logger.logWithPrefixAndValues("HEAPCHK",                                       \
                                                      "%s *** HEAP CORRUPT *** (culprit ran just before this marker)", \
                                                      (marker));                                       \
        } while (0)
#else
    #define OPENKNX_HEAPCHK(marker) \
        do                          \
        {                           \
        } while (0)
#endif
