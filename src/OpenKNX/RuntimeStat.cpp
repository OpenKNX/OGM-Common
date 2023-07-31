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

    void RuntimeStat::measureTimeBegin()
    {
        _begin_us = micros();
    }

    void RuntimeStat::measureTimeEnd()
    {
        const uint32_t end_us = micros();
        const uint32_t duration_us = end_us - _begin_us;

        uint8_t i = 0;
        while (_timeRangeMax[i] < duration_us)
        {
            i++;
        }
        _countRange[i]++;

        // TODO check removal (is redundant)
        _count++;
    }

    void RuntimeStat::showStat()
    {
        // logInfo("Stat...");
        openknx.logger.logWithPrefixAndValues("RuntimeStat", "count=%d", _count);
        for (size_t i = 0; i < 16; i++)
        {
            openknx.logger.logWithPrefixAndValues(
                "       Stat", "<=%dus: %d", 
                _timeRangeMax[i],
                _countRange[i]
            );
        }
        
    }

} // namespace OpenKNX