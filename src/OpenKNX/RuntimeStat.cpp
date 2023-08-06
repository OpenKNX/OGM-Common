#include "OpenKNX/RuntimeStat.h"

#include "OpenKNX/Facade.h"
// TODO/FIXME Logger.h is not working without openknx// TODO check 

#include "OpenKNX/Log/Logger.h"

namespace OpenKNX
{

    const uint32_t RuntimeStat::_timeRangeMax[] = {
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

    void RuntimeStat::measureTimeBegin()
    {
        _begin_us = micros();

        // measure waiting-time between two loops
        if (_end_us > 0)
        {
            const uint32_t waiting_duration_us = _begin_us - _end_us;

            _countWaitDuration[calcBucketIndex(waiting_duration_us)]++;

            _waitDurationMax_us = MAX(_waitDurationMax_us, waiting_duration_us);
            _waitDurationMin_us = MIN(_waitDurationMin_us, waiting_duration_us);
        }
    }

    void RuntimeStat::measureTimeEnd()
    {
        // store end only once at the beginning, as getting the time twice might increase error
        _end_us = micros();

        const uint32_t duration_us = _end_us - _begin_us;
        _countRunDuration[calcBucketIndex(duration_us)]++;

        _runDurationMax_us = MAX(_runDurationMax_us, duration_us);
        _runDurationMin_us = MIN(_runDurationMin_us, duration_us);

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
        openknx.logger.logWithPrefixAndValues(label, "us     min     %10d %10d",
            _runDurationMin_us, _waitDurationMin_us
        );
        openknx.logger.logWithPrefixAndValues(label, "us     max     %10d %10d",
            _runDurationMax_us, _waitDurationMax_us
        );
        for (size_t i = 0; i < 16; i++)
        {
            openknx.logger.logWithPrefixAndValues(label, "#<= %6d us  %10d %10d",
                _timeRangeMax[i], _countRunDuration[i], _countWaitDuration[i]
            );
        }
        
    }

} // namespace OpenKNX