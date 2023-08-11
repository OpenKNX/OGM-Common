#include "OpenKNX/RuntimeStat.h"

#include "OpenKNX/Facade.h"
// TODO/FIXME Logger.h is not working without openknx// TODO check 

// TODO/LIMITATION: Counter Overflow ist possible and will result in wrong results (for long? runtimes)!

#include "OpenKNX/Log/Logger.h"

namespace OpenKNX
{

    uint32_t RuntimeStat::_timeRangeMax[OPENKNX_RUNTIME_STAT_BUCKETN] = {
        OPENKNX_RUNTIME_STAT_BUCKETS,
        0xffffffff, // max value, so we do not need a special case
    };

    uint8_t RuntimeStat::calcBucketIndex(const uint32_t value_us)
    {
        uint8_t i = 0;
        while (_timeRangeMax[i] < value_us)
        {
            i++;
        }
        return i;
    }

    uint32_t RuntimeStat::calcBucketMax(const uint8_t bucketIndex, const uint32_t overallMax)
    {
        const uint32_t bucketMax = _timeRangeMax[bucketIndex];
        return MIN(bucketMax, overallMax);
    }

    uint32_t RuntimeStat::calcBucketMin(const uint8_t bucketIndex, const uint32_t overallMin)
    {
        const uint32_t bucketMin = bucketIndex==0 ? 0 : _timeRangeMax[bucketIndex-1];
        return MAX(bucketMin, overallMin);
    }

    uint32_t RuntimeStat::estimateMedian(DurationStatistic &stat)
    {
        // TODO special handling of edge-cases!

        if (_count == 0)
            return stat.durationMin_us;

        if (_count <= 2)
            return (stat.durationMin_us + stat.durationMax_us) / 2;

        uint8_t medianIndex = 0;
        const uint32_t medianCount = _count / 2;
        uint32_t cumulatedCountLower = 0;
        uint32_t cumulatedCountUpper = 0;
        for (size_t i = 0; i < OPENKNX_RUNTIME_STAT_BUCKETN; i++)
        {
            cumulatedCountUpper += stat.durationBucket[i];
            if (cumulatedCountUpper >= medianCount)
            {
                // found the bucket containing the value
                medianIndex = i;
                break;
            }
            cumulatedCountLower = cumulatedCountUpper;
        }

        // median must be in the closed interval defined by the intersection of selected bucket and [min;max]
        const uint32_t medianMin = calcBucketMin(medianIndex, stat.durationMin_us);
        const uint32_t medianMax = calcBucketMax(medianIndex, stat.durationMax_us);

        // "The ``best'' estimate for the mean [and median] is obtained by assuming the data is uniformly spread within each interval"
        // [http://www.cs.uni.edu/~campbell/stat/histrev2.html accessed 2023-08-06]
        // Using information of min- and maximum value can reduce the interval and thereby improve the result.
        double factor = 1.0 * (medianCount - cumulatedCountLower) / stat.durationBucket[medianIndex];
        return  medianMin + (medianMax - medianMin) * factor;

        // TODO check usage of one side open interval?
    }

    void RuntimeStat::measureTimeBegin()
    {
        _begin_us = micros();

        // measure waiting-time between two loops
        if (_end_us > 0)
        {
            const uint32_t waiting_duration_us = _begin_us - _end_us;

            _wait.durationBucket[calcBucketIndex(waiting_duration_us)]++;

            _wait.durationMax_us = MAX(_wait.durationMax_us, waiting_duration_us);
            _wait.durationMin_us = MIN(_wait.durationMin_us, waiting_duration_us);
            _wait.sum_us += waiting_duration_us;
        }
    }

    void RuntimeStat::measureTimeEnd()
    {
        // store end only once at the beginning, as getting the time twice might increase error
        _end_us = micros();

        const uint32_t duration_us = _end_us - _begin_us;
        _run.durationBucket[calcBucketIndex(duration_us)]++;

        _run.durationMax_us = MAX(_run.durationMax_us, duration_us);
        _run.durationMin_us = MIN(_run.durationMin_us, duration_us);
        _run.sum_us += duration_us;

        // TODO check removal (is included in _countRunDuration)
        _count++;
    }

    void RuntimeStat::showStat(std::string label)
    {
        const uint32_t runCount = _count;
        const uint32_t waitCount = _count - 1;

        openknx.logger.logWithPrefixAndValues(label, "#    count     %10d %10d",
            runCount, waitCount
        );
        openknx.logger.logWithPrefixAndValues(label, "#      sum     %10d %10d",
            _run.sum_us, _wait.sum_us
        );
        openknx.logger.logWithPrefixAndValues(label, "us     min     %10d %10d",
            _run.durationMin_us, _wait.durationMin_us
        );
        openknx.logger.logWithPrefixAndValues(label, "us     avg     %10d %10d",
            _run.sum_us / runCount,
            _wait.sum_us / waitCount
        );
        openknx.logger.logWithPrefixAndValues(label, "us    ~med     %10d %10d",
            estimateMedian(_run),
            estimateMedian(_wait)
        );
        openknx.logger.logWithPrefixAndValues(label, "us     max     %10d %10d",
            _run.durationMax_us, _wait.durationMax_us
        );
        for (size_t i = 0; i < OPENKNX_RUNTIME_STAT_BUCKETN; i++)
        {
            openknx.logger.logWithPrefixAndValues(label, "#<= %6d us  %10d %10d",
                _timeRangeMax[i], _run.durationBucket[i], _wait.durationBucket[i]
            );
        }
        
    }

} // namespace OpenKNX