/**
 * @file        MemDiag.h
 * @brief       Device-wide part of "mem stat": lowest heap since start, history since start, event log across restarts, lwIP pools
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Fed by every MemStat sample; fixed size, no heap. The event log sits in RAM a restart does not clear (power loss does).
 */
#pragma once

#ifdef OPENKNX_MEM_STAT
    #include "OpenKNX/Helper.h"
    #include <Arduino.h>
    #include <climits>
    #include <cstdint>
    #if defined(ARDUINO_ARCH_ESP32)
        #include <esp_heap_caps.h>
    #elif defined(ARDUINO_ARCH_RP2040)
        #include "pico/platform.h"
    #else
        #error "OPENKNX_MEM_STAT: ESP32 and RP2040 only"
    #endif

    #ifndef OPENKNX_MEM_STAT_EVERY
        #define OPENKNX_MEM_STAT_EVERY 8 // every n-th call of a measuring point is measured
    #endif
static_assert(OPENKNX_MEM_STAT_EVERY >= 1 && OPENKNX_MEM_STAT_EVERY <= 128 && !(OPENKNX_MEM_STAT_EVERY & (OPENKNX_MEM_STAT_EVERY - 1)), "OPENKNX_MEM_STAT_EVERY: a power of two up to 128");
    #ifndef OPENKNX_MEM_STAT_SLOW_LOOP_MS
        #define OPENKNX_MEM_STAT_SLOW_LOOP_MS 500 // a measured section that runs this long is logged as an event
    #endif

namespace OpenKNX
{
    namespace Stat
    {
        /** @brief Free heap now: ESP32 internal 8-bit heap, RP2040 the newlib heap (both cores). */
        inline int32_t freeHeap()
        {
    #if defined(ARDUINO_ARCH_ESP32)
            return (int32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    #else
            return (int32_t)rp2040.getFreeHeap();
    #endif
        }

        class MemDiag
        {
          public:
            // Measuring point ids: fixed points, the rest of the loop, then modules on the loop core and the other core
            static constexpr uint8_t ID_LOOP = 0, ID_CONSOLE = 1, ID_KNX = 2, ID_GPIO = 3, ID_TIME = 4, ID_SUN = 5, ID_MODULES = 6, ID_OTHER = 7, ID_SETUP = 8;
            static constexpr uint8_t ID_MODULE0 = 16, ID_MODULE1 = 48, ID_EXTRA = 80, ID_NONE = 0xFF; // ID_EXTRA: points a module adds
            static constexpr uint8_t ID_LWIP_HEAP = 0xFE; // EV_LWIP id of lwIP's own heap; pools are 0..n
            static constexpr uint8_t ID_NET = 96;         // 96..111: steps inside the network stack (MEM_CRUMB), labels in Common::memLabel

            enum Event : uint8_t
            {
                EV_NONE = 0,
                EV_BOOT,        // value: reset reason, id: point that ran last before it
                EV_NEW_FAILED,  // operator new found no memory; value: free heap. RP2040: a restart follows; ESP32: bad_alloc as before
                EV_HEAP_LOW,    // new lowest free heap since start, 1 KiB below the last one noted; value: free heap
                EV_LWIP,        // an lwIP pool refused an allocation; id: pool, value: its error count
                EV_TCP_DROP,    // not logged: TCP drops are mostly closed ports and refused data, they stay in the history
                EV_WEB_REFUSED, // web server turned connections away; value: how many
                EV_SLOW,        // a measured section ran longer than OPENKNX_MEM_STAT_SLOW_LOOP_MS; value: longest ms
            };

            struct Entry // 12 B
            {
                uint32_t t;     // uptime in s
                uint32_t value;
                uint8_t code;
                uint8_t id;
                uint8_t boot;   // restart counter, so entries of an earlier run are told apart
                uint8_t core;
            };

            struct Bucket // 12 B
            {
                uint16_t minFree; // in 16 B units, EMPTY without a sample
                uint16_t maxFree;
                uint16_t slowMs;  // longest measured section, ms (65535 = longer)
                uint8_t minId;    // point that measured the minimum
                uint8_t slowId;
                uint8_t lwipErr;  // new lwIP pool errors (saturating)
                uint8_t tcpDrop;  // new TCP drops (saturating)
                uint8_t web;      // connections the web server turned away (saturating)
            };

            static constexpr uint8_t HIST = 64;
            static constexpr uint8_t LOG = 32;
            static constexpr uint8_t RECENT = 16;  // minutes kept at 1 min resolution
            static constexpr uint32_t SPAN0 = 60;  // s per bucket before the first merge
            static constexpr uint16_t EMPTY = 0xFFFF;

            void boot();
            void loop();

            /** @brief Core of the caller. */
            static inline uint8_t coreNum()
            {
    #if defined(ARDUINO_ARCH_RP2040)
                return (uint8_t)get_core_num();
    #else
                return (uint8_t)xPortGetCoreID();
    #endif
            }

            /** @brief Marks the point that runs now on the loop core: the one a restart is blamed on. Returns the one before. */
            inline uint8_t running(uint8_t id)
            {
                const uint8_t prev = _ram.lastId;
                if (coreNum() == _loopCore) _ram.lastId = id;
                return prev;
            }
            void sample(int32_t free, uint8_t id, uint32_t t);
            void slow(uint8_t id, uint32_t us);
            /** @brief From the web server's lwIP callback: only counts, loop() books it. */
            inline void webRefused() { webRefusedTotal++; }
            void log(uint8_t code, uint8_t id, uint32_t value);

            // --- read side for the console ---
            int32_t lowFree = INT32_MAX;
            uint32_t lowAt = 0;
            uint8_t lowId = ID_NONE;
            uint8_t lowCore = 0;
            volatile uint32_t webRefusedTotal = 0;

            uint8_t histCount() const { return _histN; }
            uint32_t histSpan() const { return _span; }
            uint32_t histStart() const { return _histT0; }
            const Bucket &hist(uint8_t i) const { return _hist[i < HIST ? i : HIST - 1]; }
            uint8_t logCount() const;
            const Entry &logEntry(uint8_t newestFirst) const;
            uint8_t bootCount() const { return _ram.boot; }
            int32_t lowRecent(uint8_t minutes) const;                // the last minutes at 1 min resolution; INT32_MAX when none
            int32_t lowSince(uint32_t seconds, uint32_t &covered) const; // from the history: `covered` = what the spans really span
            static int32_t largestFreeBlock();                       // -1 where the platform has no cheap way

            // lwIP: pools need LWIP_STATS + MEMP_STATS (RP2040 only: ESP-IDF takes its pools from the heap)
            static uint8_t lwipPools();
            static const char *lwipPoolName(uint8_t i);
            static void lwipPool(uint8_t i, uint16_t &used, uint16_t &max, uint16_t &avail, uint16_t &err);
            static bool lwipHeap(uint32_t &used, uint32_t &max, uint32_t &avail, uint16_t &err); // lwIP's own heap (PBUF_RAM)
            static bool lwipTcp(uint16_t &drop, uint16_t &memErr, uint16_t &rtErr, uint16_t &xmit, uint16_t &recv);

          private:
            struct Ram // survives a restart, not a power loss
            {
                uint32_t magic;
                uint8_t head;
                uint8_t count;
                uint8_t boot;
                uint8_t lastId;
                Entry e[LOG];
            };
            static Ram _ram;

            Bucket _hist[HIST] = {};
            uint16_t _recent[RECENT] = {};
            uint32_t _recentMinute = 0;
            uint8_t _histN = 0;
            uint8_t _loopCore = 0; // the core Common::loop runs on: the only writer of the buckets
            uint32_t _span = SPAN0;
            uint32_t _histT0 = 0;
            uint32_t _lastT = 0;
            uint32_t _lastPoll = 0;
            uint32_t _lwipErrSeen = 0;
            uint32_t _tcpDropSeen = 0;
            uint16_t _poolErr[24] = {};
            uint16_t _heapErr = 0;
            uint32_t _webSeen = 0;
            int32_t _lowLogged = INT32_MAX;

            Bucket &current(uint32_t t);
            void recent(uint32_t t, uint16_t q);
            static void newFailed();
        };

        extern MemDiag memDiag;

        /** @brief Marks a step as the point that runs (the watchdog record names it) and restores the previous one at scope end. */
        struct MemCrumb
        {
            uint8_t prev;
            explicit MemCrumb(uint8_t id) : prev(memDiag.running(id)) {}
            ~MemCrumb() { memDiag.running(prev); }
        };
    } // namespace Stat
} // namespace OpenKNX
#endif
