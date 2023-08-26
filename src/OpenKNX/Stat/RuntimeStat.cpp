#include "OpenKNX/Stat/RuntimeStat.h"

#include "OpenKNX/Facade.h"
// TODO/FIXME Logger.h is not working without openknx// TODO check 

// TODO/LIMITATION: Counter Overflow ist possible and will result in wrong results (for long? runtimes)!

#include "OpenKNX/Log/Logger.h"

namespace OpenKNX
{
    namespace Stat
    {

        void RuntimeStat::measureTimeBegin()
        {
            _begin_us = micros();

            // measure waiting-time between two loops
            if (_end_us > 0)
            {
                _wait.measure(_begin_us - _end_us);
            }
        }

        void RuntimeStat::measureTimeEnd()
        {
            // store end only once at the beginning, as getting the time twice might increase error
            _end_us = micros();

            _run.measure(_end_us - _begin_us);
        }

        void RuntimeStat::showStat(std::string label)
        {
            const uint32_t runCount = _run._count;
            const uint32_t waitCount = _wait._count;

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
                _run.estimateMedian(),
                _wait.estimateMedian()
            );
            openknx.logger.logWithPrefixAndValues(label, "us     max     %10d %10d",
                _run.durationMax_us, _wait.durationMax_us
            );
            for (size_t i = 0; i < OPENKNX_RUNTIME_STAT_BUCKETN; i++)
            {
                openknx.logger.logWithPrefixAndValues(label, "#<= %6d us  %10d %10d",
                    DurationStatistic::_timeRangeMax[i], _run.durationBucket[i], _wait.durationBucket[i]
                );
            }
            
        }
    } // namespace Stat
} // namespace OpenKNX