#pragma once

#include <Arduino.h>
#include <string>

#ifdef OPENKNX_RUNTIME_STAT
  #define RUNTIME_MEASURE_BEGIN(X) (X)->measureTimeBegin();
  #define RUNTIME_MEASURE_END(X) (X)->measureTimeEnd();
#else
  #define RUNTIME_MEASURE_BEGIN(X)
  #define RUNTIME_MEASURE_END(X)
#endif

#ifndef OPENKNX_RUNTIME_STAT_BUCKETS
    #define OPENKNX_RUNTIME_STAT_BUCKETS 50, 100, 200, 400, 600, 800, 1000, 1500, 2000, 3000, 4000, 5000, 6000, 7000, 10000
#endif


namespace OpenKNX
{
    const static uint32_t runtime_stat_timeRangeMax[] = {
        OPENKNX_RUNTIME_STAT_BUCKETS,
        0xffffffff, // max value, so we do not need a special case
    };

    class RuntimeStat
    {
      private:
        // const static uint32_t _timeRangeMax[16];

        uint32_t _begin_us = 0;
        uint32_t _end_us = 0;

        uint32_t _count = 0;
        uint32_t _countRunDuration[sizeof(_timeRangeMax)] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        uint32_t _countWaitDuration[sizeof(_timeRangeMax)] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };

        // TODO use own structure for this
        uint32_t _runDurationMin_us = 0xffffffffu;
        uint32_t _runDurationMax_us = 0;
        uint64_t _runSum_us = 0;
        uint32_t _waitDurationMin_us = 0xffffffffu;
        uint32_t _waitDurationMax_us = 0;
        uint64_t _waitSum_us = 0;
        
        static uint8_t calcBucketIndex(const uint32_t value_us);

        uint32_t calcBucketMax(const uint8_t bucketIndex, const uint32_t overallMax);
        uint32_t calcBucketMin(const uint8_t bucketIndex, const uint32_t overallMin);
        uint32_t estimateMedian();

      protected:

      public:
        void measureTimeBegin();
        void measureTimeEnd();
        void showStat(std::string label);
    };
} // namespace OpenKNX