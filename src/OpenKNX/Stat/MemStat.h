/**
 * @file        MemStat.h
 * @brief       Heap statistics per measuring point, the memory counterpart of RuntimeStat (console "mem stat")
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Like RUNTIME_MEASURE_BEGIN/END: MEM_STAT_BEGIN/END around any code, MEM_STAT_PHASE_BEGIN/END around a start phase.
 * The heap is global: what core 1 or another task takes meanwhile counts for the point that runs.
 */
#pragma once

#ifdef OPENKNX_MEM_STAT
    #include "OpenKNX/Stat/MemDiag.h"

namespace OpenKNX
{
    namespace Stat
    {
        class MemStat
        {
          public:
            int32_t start = 0;       // taken by the start phases (init, setup, processAfterStartupDelay)
            int32_t keptMax = 0;     // most one measured call kept (free before - free after)
            uint32_t keptMaxAt = 0;  // uptime in s
            int32_t freedMax = 0;    // most one measured call gave back
            int64_t keptSum = 0;     // net over all measured calls; / samples = average
            int32_t low = INT32_MAX; // lowest free heap right after a measured call
            uint32_t lowAt = 0;      // uptime in s
            uint32_t samples = 0;    // measured calls
            uint8_t id = MemDiag::ID_NONE;
            uint8_t every = OPENKNX_MEM_STAT_EVERY - 1; // 0 = every call, for rare and heavy calls

            inline void phaseBegin()
            {
                _parent = memDiag.running(id);
                _before = freeHeap();
            }
            inline void phaseEnd()
            {
                start += _before - freeHeap();
                memDiag.running(_parent);
            }

            inline void begin()
            {
                _parent = memDiag.running(id);
                if (++_tick & every)
                    return;
                _before = freeHeap();
                _armed = true;
            }

            /** @brief Returns what the call kept, or 0 when this call was not measured. */
            inline int32_t end()
            {
                memDiag.running(_parent); // what runs after this belongs to the enclosing point again
                if (!_armed)
                    return 0;
                _armed = false;
                const int32_t now = freeHeap();
                const int32_t kept = _before - now;
                const uint32_t t = uptime();
                samples++;
                keptSum += kept;
                if (kept > keptMax)
                {
                    keptMax = kept;
                    keptMaxAt = t;
                }
                if (-kept > freedMax)
                    freedMax = -kept;
                if (now < low)
                {
                    low = now;
                    lowAt = t;
                }
                memDiag.sample(now, id, t);
                return kept;
            }

            /** @brief Clears the call values; the start value stays. */
            void reset()
            {
                keptMax = 0;
                keptMaxAt = 0;
                freedMax = 0;
                keptSum = 0;
                low = INT32_MAX;
                lowAt = 0;
                samples = 0;
            }

          private:
            int32_t _before = 0;
            uint8_t _parent = MemDiag::ID_NONE;
            uint8_t _tick = 0;
            bool _armed = false;
        };
    } // namespace Stat
} // namespace OpenKNX

    #define MEM_STAT_PHASE_BEGIN(X) (X).phaseBegin();
    #define MEM_STAT_PHASE_END(X) (X).phaseEnd();
    #define MEM_STAT_BEGIN(X) (X).begin();
    #define MEM_STAT_END(X) (X).end();
    #define MEM_CRUMB_CAT2(A, B) A##B
    #define MEM_CRUMB_CAT(A, B) MEM_CRUMB_CAT2(A, B)
    #define MEM_CRUMB(STEP) OpenKNX::Stat::MemCrumb MEM_CRUMB_CAT(_memCrumb, __LINE__)(OpenKNX::Stat::MemDiag::ID_NET + (STEP));
    #define MEM_STAT_END_WARN(X, WARN_BYTES, WARN_LABEL)                                                                           \
        {                                                                                                                          \
            const int32_t kept_b = (X).end();                                                                                      \
            if (kept_b > (WARN_BYTES))                                                                                             \
                openknx.logger.logWithPrefixAndValues(WARN_LABEL, "Warning: MemStat kept %d > %d bytes", (int)kept_b, (int)(WARN_BYTES)); \
        }
#else
    #define MEM_CRUMB(STEP)
    #define MEM_STAT_PHASE_BEGIN(X)
    #define MEM_STAT_PHASE_END(X)
    #define MEM_STAT_BEGIN(X)
    #define MEM_STAT_END(X)
    #define MEM_STAT_END_WARN(X, WARN_BYTES, WARN_LABEL)
#endif
