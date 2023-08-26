#pragma once

#include <Arduino.h>
#include <string>
#include "DurationStatistic.h"

#ifdef OPENKNX_RUNTIME_STAT
  #define RUNTIME_MEASURE_BEGIN(X) (X).measureTimeBegin();
  #define RUNTIME_MEASURE_END(X) (X).measureTimeEnd();
#else
  #define RUNTIME_MEASURE_BEGIN(X)
  #define RUNTIME_MEASURE_END(X)
#endif

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
          void measureTimeBegin();
          void measureTimeEnd();
          void showStat(std::string label);
      };
  } // namespace Stat
} // namespace OpenKNX