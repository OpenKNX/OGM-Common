/**
 * @file        MemDiag.cpp
 * @brief       Lowest heap, history since start, event log across restarts and lwIP pools for "mem stat"
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#include "OpenKNX/Stat/MemDiag.h"

#ifdef OPENKNX_MEM_STAT
    #include "lwip/opt.h"
    #include <cstring>
    #include <new>
    #if defined(ARDUINO_ARCH_RP2040)
        #include "pico/platform.h"
    #else
        #include "sdkconfig.h"
        #include <esp_attr.h>
        #include <esp_system.h>
        #if defined(OPENKNX_LWIP_STATS) && !CONFIG_LWIP_STATS
            #error "OPENKNX_LWIP_STATS on ESP32 needs CONFIG_LWIP_STATS=y (custom_sdkconfig)"
        #endif
    #endif
    #if LWIP_STATS
        #include "lwip/memp.h"
        #include "lwip/stats.h"
    #endif

namespace OpenKNX
{
    namespace Stat
    {
        MemDiag memDiag;

    #if defined(ARDUINO_ARCH_RP2040)
        __attribute__((section(".uninitialized_data.memdiag"))) MemDiag::Ram MemDiag::_ram;
        static inline uint8_t resetReason() { return (uint8_t)rp2040.getResetReason(); }
    #else
        __NOINIT_ATTR MemDiag::Ram MemDiag::_ram;
        static inline uint8_t resetReason() { return (uint8_t)esp_reset_reason(); }
    #endif

        static constexpr uint32_t MAGIC = 0x4D454D44; // "MEMD"

        static void initBucket(MemDiag::Bucket &b)
        {
            b = {};
            b.minFree = MemDiag::EMPTY;
            b.minId = MemDiag::ID_NONE;
            b.slowId = MemDiag::ID_NONE;
        }

        static inline uint8_t sat8(uint32_t v) { return v > 255 ? 255 : (uint8_t)v; }
        static inline uint16_t quant(int32_t free) { return free <= 0 ? 0 : (free / 16 >= MemDiag::EMPTY ? MemDiag::EMPTY - 1 : (uint16_t)(free / 16)); }

        // Runs from Common::init(), on the core that runs Common::loop() later
        void MemDiag::boot()
        {
            uint8_t prevId = _ram.lastId;
            if (_ram.magic != MAGIC || _ram.head >= LOG || _ram.count > LOG)
            {
                memset(&_ram, 0, sizeof(_ram));
                _ram.magic = MAGIC;
                prevId = ID_NONE;
            }
            _ram.boot++;
            _loopCore = coreNum();
            _ram.lastId = ID_SETUP; // init, setup and start-up until the first loop
            for (uint8_t i = 0; i < HIST; i++)
                initBucket(_hist[i]);
            for (uint8_t i = 0; i < RECENT; i++)
                _recent[i] = EMPTY;
            _histT0 = _lastT = uptime();
            _recentMinute = _histT0 / 60;
            log(EV_BOOT, prevId, resetReason());
            std::set_new_handler(&MemDiag::newFailed);
        }

        // operator new found nothing. Note who ran; then what new does without a handler: bad_alloc, or the end
        void MemDiag::newFailed()
        {
            // Another core or task than the loop: the loop's point did not ask for it
            memDiag.log(EV_NEW_FAILED, coreNum() == memDiag._loopCore ? _ram.lastId : ID_NONE, (uint32_t)freeHeap());
    #if __cpp_exceptions
            throw std::bad_alloc();
    #else
            abort();
    #endif
        }

        void MemDiag::log(uint8_t code, uint8_t id, uint32_t value)
        {
            const uint32_t t = uptime();
            // The same event of the same point within 10 s stays one entry: a burst must not push the earlier run out
            for (uint8_t k = 0; k < _ram.count && k < 4 && code != EV_BOOT; k++)
            {
                Entry &l = _ram.e[(_ram.head + LOG - 1 - k) % LOG];
                if (l.code == code && (l.id == id || code == EV_NEW_FAILED) && l.boot == _ram.boot && t - l.t <= 10)
                {
                    if (code == EV_WEB_REFUSED)
                        l.value += value;
                    else if (code == EV_SLOW)
                        l.value = value > l.value ? value : l.value;
                    else
                        l.value = value;
                    l.t = t;
                    return;
                }
            }
            Entry &e = _ram.e[_ram.head];
            e.t = t;
            e.value = value;
            e.code = code;
            e.id = id;
            e.boot = _ram.boot;
            e.core = coreNum();
            _ram.head = (uint8_t)((_ram.head + 1) % LOG);
            if (_ram.count < LOG) _ram.count++;
        }

        uint8_t MemDiag::logCount() const { return _ram.count; }

        const MemDiag::Entry &MemDiag::logEntry(uint8_t newestFirst) const
        {
            return _ram.e[(_ram.head + LOG - 1 - newestFirst % LOG) % LOG];
        }

        // Loop core only: the buckets have one writer
        MemDiag::Bucket &MemDiag::current(uint32_t t)
        {
            if (t < _lastT) t = _lastT; // uptime() may step back once at the millis() wrap
            _lastT = t;
            const uint32_t rel = t - _histT0;
            while (rel / _span >= HIST)
            {
                // Full: pairs become one bucket, the span doubles; the history still reaches back to the start
                for (uint8_t i = 0; i < HIST / 2; i++)
                {
                    Bucket m;
                    initBucket(m);
                    for (uint8_t j = 2 * i; j <= 2 * i + 1 && j < _histN; j++)
                    {
                        const Bucket &b = _hist[j];
                        if (b.minFree < m.minFree)
                        {
                            m.minFree = b.minFree;
                            m.minId = b.minId;
                        }
                        if (b.maxFree > m.maxFree) m.maxFree = b.maxFree;
                        if (b.slowId != ID_NONE && (m.slowId == ID_NONE || b.slowMs > m.slowMs))
                        {
                            m.slowMs = b.slowMs;
                            m.slowId = b.slowId;
                        }
                        m.lwipErr = sat8((uint32_t)m.lwipErr + b.lwipErr);
                        m.tcpDrop = sat8((uint32_t)m.tcpDrop + b.tcpDrop);
                        m.web = sat8((uint32_t)m.web + b.web);
                    }
                    _hist[i] = m;
                }
                for (uint8_t i = HIST / 2; i < HIST; i++)
                    initBucket(_hist[i]);
                _histN = (uint8_t)((_histN + 1) / 2);
                _span *= 2;
            }
            const uint8_t idx = (uint8_t)(rel / _span);
            while (_histN <= idx)
                initBucket(_hist[_histN++]);
            return _hist[idx];
        }

        // The last RECENT minutes at 1 min resolution, so "the last minute" stays a minute however long the device runs
        void MemDiag::recent(uint32_t t, uint16_t q)
        {
            const uint32_t minute = t / 60;
            if (minute > _recentMinute)
            {
                for (uint32_t m = _recentMinute + 1; m <= minute && m <= _recentMinute + RECENT; m++)
                    _recent[m % RECENT] = EMPTY;
                _recentMinute = minute;
            }
            if (minute == _recentMinute && q < _recent[minute % RECENT]) _recent[minute % RECENT] = q;
        }

        void MemDiag::sample(int32_t free, uint8_t id, uint32_t t)
        {
            if (coreNum() != _loopCore) return;
            if (free < lowFree)
            {
                lowFree = free;
                lowAt = t;
                lowId = id;
                lowCore = _loopCore;
            }
            const uint16_t q = quant(free);
            Bucket &b = current(t);
            if (q < b.minFree)
            {
                b.minFree = q;
                b.minId = id;
            }
            if (q > b.maxFree) b.maxFree = q;
            recent(_lastT, q);
        }

        void MemDiag::slow(uint8_t id, uint32_t us)
        {
            if (coreNum() != _loopCore) return;
            const uint32_t ms = us / 1000;
            const uint16_t ms16 = ms > 0xFFFF ? 0xFFFF : (uint16_t)ms;
            Bucket &b = current(uptime());
            if (b.slowId == ID_NONE || ms16 > b.slowMs)
            {
                b.slowMs = ms16;
                b.slowId = id;
            }
            if (ms >= OPENKNX_MEM_STAT_SLOW_LOOP_MS) log(EV_SLOW, id, ms);
        }

        void MemDiag::loop()
        {
            const uint32_t t = uptime();
            if (t == _lastPoll || coreNum() != _loopCore) return;
            _lastPoll = t;
            Bucket &b = current(t); // a bucket also for a second without samples

            // Each pool error is an event; the bucket holds the sum
            uint32_t errSum = 0;
            for (uint8_t i = 0; i < lwipPools(); i++)
            {
                uint16_t used, max, avail, err;
                lwipPool(i, used, max, avail, err);
                errSum += err;
                if (i < sizeof(_poolErr) / sizeof(_poolErr[0]) && err != _poolErr[i])
                {
                    _poolErr[i] = err;
                    log(EV_LWIP, i, err);
                }
            }
            uint32_t hu, hm, ha;
            uint16_t herr = 0;
            if (lwipHeap(hu, hm, ha, herr))
            {
                errSum += herr;
                if (herr != _heapErr)
                {
                    _heapErr = herr;
                    log(EV_LWIP, ID_LWIP_HEAP, herr);
                }
            }
            if (errSum > _lwipErrSeen) b.lwipErr = sat8((uint32_t)b.lwipErr + errSum - _lwipErrSeen);
            _lwipErrSeen = errSum;

            // TCP drops: history only, see EV_TCP_DROP
            uint16_t drop, memErr, rtErr, xmit, recv;
            if (lwipTcp(drop, memErr, rtErr, xmit, recv))
            {
                if (drop > _tcpDropSeen) b.tcpDrop = sat8((uint32_t)b.tcpDrop + drop - _tcpDropSeen);
                _tcpDropSeen = drop;
            }

            // Counted in the lwIP callback, booked here in the loop
            const uint32_t web = webRefusedTotal;
            if (web != _webSeen)
            {
                b.web = sat8((uint32_t)b.web + (web - _webSeen));
                log(EV_WEB_REFUSED, ID_NONE, web - _webSeen);
                _webSeen = web;
            }

            // A new low more than 1 KiB under the last one noted
            if (lowFree != INT32_MAX && lowFree + 1024 <= _lowLogged)
            {
                _lowLogged = lowFree;
                log(EV_HEAP_LOW, lowId, (uint32_t)lowFree);
            }
        }

        int32_t MemDiag::lowRecent(uint8_t minutes) const
        {
            const uint32_t now = uptime() / 60;
            int32_t low = INT32_MAX;
            for (uint32_t k = 0; k <= minutes && k < RECENT && k <= now; k++) // the running minute plus `minutes` full ones
            {
                const uint32_t m = now - k;
                if (m > _recentMinute || m + RECENT <= _recentMinute) continue; // not written yet, or already reused
                const uint16_t q = _recent[m % RECENT];
                if (q != EMPTY && (int32_t)q * 16 < low) low = (int32_t)q * 16;
            }
            return low;
        }

        int32_t MemDiag::lowSince(uint32_t seconds, uint32_t &covered) const
        {
            covered = 0;
            if (!_histN) return INT32_MAX;
            const uint32_t now = uptime();
            const uint32_t from = now > seconds ? now - seconds : 0;
            int32_t low = INT32_MAX;
            for (int i = _histN - 1; i >= 0; i--)
            {
                const uint32_t bStart = _histT0 + (uint32_t)i * _span;
                if (bStart + _span <= from) break;
                covered = now > bStart ? now - bStart : 0;
                if (_hist[i].minFree != EMPTY && (int32_t)_hist[i].minFree * 16 < low) low = (int32_t)_hist[i].minFree * 16;
            }
            return low;
        }

        int32_t MemDiag::largestFreeBlock()
        {
    #if defined(ARDUINO_ARCH_ESP32)
            return (int32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    #else
            return -1;
    #endif
        }

    #if LWIP_STATS && MEMP_STATS
        static const char *const kPoolNames[] = {
        #define LWIP_MEMPOOL(name, num, size, desc) #name,
        #include "lwip/priv/memp_std.h"
        };

        uint8_t MemDiag::lwipPools() { return (uint8_t)MEMP_MAX; }
        const char *MemDiag::lwipPoolName(uint8_t i) { return i < MEMP_MAX ? kPoolNames[i] : "?"; }

        void MemDiag::lwipPool(uint8_t i, uint16_t &used, uint16_t &max, uint16_t &avail, uint16_t &err)
        {
            const struct stats_mem *s = i < MEMP_MAX ? lwip_stats.memp[i] : nullptr;
            used = s ? (uint16_t)s->used : 0;
            max = s ? (uint16_t)s->max : 0;
            avail = s ? (uint16_t)s->avail : 0;
            err = s ? (uint16_t)s->err : 0;
        }
    #else
        uint8_t MemDiag::lwipPools() { return 0; }
        const char *MemDiag::lwipPoolName(uint8_t) { return "?"; }
        void MemDiag::lwipPool(uint8_t, uint16_t &used, uint16_t &max, uint16_t &avail, uint16_t &err) { used = max = avail = err = 0; }
    #endif

        bool MemDiag::lwipHeap(uint32_t &used, uint32_t &max, uint32_t &avail, uint16_t &err)
        {
    #if LWIP_STATS && MEM_STATS
            used = lwip_stats.mem.used;
            max = lwip_stats.mem.max;
            avail = lwip_stats.mem.avail;
            err = lwip_stats.mem.err;
            return true;
    #else
            used = max = avail = 0;
            err = 0;
            return false;
    #endif
        }

        bool MemDiag::lwipTcp(uint16_t &drop, uint16_t &memErr, uint16_t &rtErr, uint16_t &xmit, uint16_t &recv)
        {
    #if LWIP_STATS && TCP_STATS
            drop = lwip_stats.tcp.drop;
            memErr = lwip_stats.tcp.memerr;
            rtErr = lwip_stats.tcp.rterr;
            xmit = lwip_stats.tcp.xmit;
            recv = lwip_stats.tcp.recv;
            return true;
    #else
            drop = memErr = rtErr = xmit = recv = 0;
            return false;
    #endif
        }
    } // namespace Stat
} // namespace OpenKNX
#endif
