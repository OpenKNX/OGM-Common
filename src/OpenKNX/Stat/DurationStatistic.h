#pragma once

#include <Arduino.h>

#ifndef OPENKNX_RUNTIME_STAT_BUCKETS
    #define OPENKNX_RUNTIME_STAT_BUCKETS 50, 100, 200, 400, 600, 800, 1000, 1500, 2000, 3000, 4000, 5000, 6000, 7000, 10000
#endif
#ifndef OPENKNX_RUNTIME_STAT_BUCKETN
    #define OPENKNX_RUNTIME_STAT_BUCKETN 16
#endif

namespace OpenKNX
{
    namespace Stat
    {

      class DurationStatistic
      {
        private:
        public:
          static uint32_t _timeRangeMax[OPENKNX_RUNTIME_STAT_BUCKETN];

          uint32_t _count = 0;
          uint64_t sum_us = 0;
          uint32_t durationMin_us = 0xffffffffu;
          uint32_t durationMax_us = 0;
          uint32_t durationBucket[OPENKNX_RUNTIME_STAT_BUCKETN];

          static uint8_t calcBucketIndex(const uint32_t value_us);

          void measure(const uint32_t duration_us);
          uint32_t calcBucketMax(const uint8_t bucketIndex);
          uint32_t calcBucketMin(const uint8_t bucketIndex);
          uint32_t estimateMedian();

      };
  } // namespace Stat
} // namespace OpenKNX