#pragma once

#include <Arduino.h>

#define RUNTIME_MEASURE_BEGIN(X) (X)->measureTimeBegin();
#define RUNTIME_MEASURE_END(X) (X)->measureTimeEnd();

namespace OpenKNX
{
    class RuntimeStat
    {
      private:
        // TODO use constant const static size_t _timeN = 7;
        const static uint32_t _timeRangeMax[16];

        uint32_t _begin_us = 0;
        uint32_t _end_us = 0;

        uint32_t _count = 0;
        uint32_t _countRunDuration[sizeof(_timeRangeMax)] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        uint32_t _countWaitDuration[sizeof(_timeRangeMax)] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        
        static uint8_t calcBucketIndex(const uint32_t value_us);

      protected:

      public:
        void measureTimeBegin();
        void measureTimeEnd();
        void showStat();
    };
} // namespace OpenKNX