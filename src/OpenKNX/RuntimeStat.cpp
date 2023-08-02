#include "OpenKNX/RuntimeStat.h"

#include "OpenKNX/Facade.h"
// TODO/FIXME Logger.h is not working without openknx// TODO check 

#include "OpenKNX/Log/Logger.h"

namespace OpenKNX
{

    const uint32_t RuntimeStat::_timeRangeMax[16] = {
        50, 
        100, 
        200, 
        400, 
        600, 
        800, 
        1000, 
        1500, 
        2000, 
        3000, 
        4000, 
        5000,
        6000,
        7000,
        10000,
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

        }
    }

    void RuntimeStat::measureTimeEnd()
    {
        // store end only once at the beginning, as getting the time twice might increase error
        _end_us = micros();

        const uint32_t duration_us = _end_us - _begin_us;
        _countRunDuration[calcBucketIndex(duration_us)]++;

        // TODO check removal (is included in _countRunDuration)
        _count++;
    }

    void RuntimeStat::showStat()
    {
        // logInfo("Stat...");
        openknx.logger.logWithPrefixAndValues(
            "       Stat", "    count     %10d %10d", 
            _count,
            _count-1
        );
        for (size_t i = 0; i < 16; i++)
        {
            openknx.logger.logWithPrefixAndValues(
                "       Stat", "<= %6d us  %10d %10d", 
                _timeRangeMax[i],
                _countRunDuration[i],
                _countWaitDuration[i]
            );
        }
        
    }

} // namespace OpenKNX