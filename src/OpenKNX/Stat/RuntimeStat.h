#pragma once

#include "DurationStatistic.h"
#include <Arduino.h>
#include <string>

#ifdef OPENKNX_RUNTIME_STAT
    #define RUNTIME_MEASURE_BEGIN(X) (X).measureTimeBegin();
    #define RUNTIME_MEASURE_END(X) (X).measureTimeEnd();
#else
    #define RUNTIME_MEASURE_BEGIN(X)
    #define RUNTIME_MEASURE_END(X)
#endif

#define OPENKNX_RUNTIME_STAT_BUCKETN1 (OPENKNX_RUNTIME_STAT_BUCKETN-1)

namespace OpenKNX
{
    namespace Stat
    {
        class RuntimeStat
        {
          private:
            uint32_t _begin_us = 0;
            uint32_t _end_us = 0;

            DurationStatistic _run;
            DurationStatistic _wait;

          public:
            static void showStatHeader();

            void measureTimeBegin();
            void measureTimeEnd();
            void showStat(std::string label, const uint8_t core = 0, const bool stat = true, const bool hist = false);
        };
    } // namespace Stat
} // namespace OpenKNX