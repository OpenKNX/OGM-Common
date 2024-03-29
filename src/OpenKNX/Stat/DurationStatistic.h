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
            /// Calculate the histogram bucket-index for a given duration.
            /// @param value_us the duration to put in histogram; unit µs
            /// @return index within interval [0; OPENKNX_RUNTIME_STAT_BUCKETN[
            static uint8_t calcBucketIndex(const uint32_t value_us);

            /// Calculate the maximum possible value within a given histogram-bucket.
            /// @param bucketIndex
            /// @return the upper limit of the bucket, or durationMax_us if within the value range of bucket
            uint32_t calcBucketMax(const uint8_t bucketIndex);

            /// Calculate the minium possible value within a given histogram-bucket.
            /// @param bucketIndex
            /// @return the lover limit of the bucket, or durationMin_us if within the value range of bucket
            uint32_t calcBucketMin(const uint8_t bucketIndex);

          public:
            // define the upper (included) limit of every time buckets. Last value must be maximum value of data-type.
            static uint32_t _timeRangeMax[OPENKNX_RUNTIME_STAT_BUCKETN];

            // the number of collected durations
            uint32_t _count = 0;

            // the overall sum of all collected durations; unit µs (microseconds)
            uint64_t sum_us = 0;

            // shortest of all collected durations; unit µs (microseconds)
            uint32_t durationMin_us = 0xffffffffu;

            // longest of all collected durations; unit µs (microseconds)
            uint32_t durationMax_us = 0;

            // the histogram data; number of collected durations within buckets (defined by upper limit, see _timeRangeMax)
            uint32_t durationBucket[OPENKNX_RUNTIME_STAT_BUCKETN];

            /// Update statistic based on a duration measurement.
            /// Will increase count, sum, update min/max and include in histogram.
            /// @param duration_us the duration; unit µs (microseconds)
            void measure(const uint32_t duration_us);

            /// @brief Calculate an average of duration.
            /// @return a duration value; unit µs (microseconds)
            uint32_t avg_us();

            /// @brief Calculate an estimation of the median duration.
            /// @return a duration value; unit µs (microseconds)
            uint32_t estimateMedian_us();

            /// @brief Get sum of all collected durations.
            /// @return a duration value; unit ms (milliseconds)
            uint32_t sum_ms();

            /// @brief Get the number of durations collected in the bucket.
            /// @param bucketIndex
            /// @return
            uint32_t getHistBucket(const uint8_t bucketIndex);

            /// @brief Get the maximum duration included in the bucket.
            /// @param bucketIndex
            /// @return a duration value; unit µs (microseconds)
            static uint32_t getHistBucketUpper_us(const uint8_t bucketIndex);
        };
    } // namespace Stat
} // namespace OpenKNX