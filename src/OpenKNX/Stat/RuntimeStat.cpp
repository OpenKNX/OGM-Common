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

        void RuntimeStat::showStatHeader()
        {
            openknx.logger.logWithPrefixAndValues("RuntimeStat", "type  param unit  value_run value_wait");
        }

        void RuntimeStat::showStat(std::string label)
        {
            openknx.logger.logWithPrefixAndValues(label, "stat  count    # %10d %10d", _run._count, _wait._count);
            openknx.logger.logWithPrefixAndValues(label, "stat    sum   us %10d %10d", _run.sum_us, _wait.sum_us);
            openknx.logger.logWithPrefixAndValues(label, "stat    min   us %10d %10d", _run.durationMin_us, _wait.durationMin_us);
            openknx.logger.logWithPrefixAndValues(label, "stat    avg   us %10d %10d", _run.avg_us(), _wait.avg_us());
            openknx.logger.logWithPrefixAndValues(label, "stat   ~med   us %10d %10d", _run.estimateMedian_us(), _wait.estimateMedian_us());
            openknx.logger.logWithPrefixAndValues(label, "stat    max   us %10d %10d", _run.durationMax_us, _wait.durationMax_us);
            for (size_t i = 0; i < OPENKNX_RUNTIME_STAT_BUCKETN; i++)
            {
                openknx.logger.logWithPrefixAndValues(label, "hist %6d   us %10d %10d", DurationStatistic::getHistBucketUpper(i), _run.getHistBucket(i), _wait.getHistBucket(i));
            }
        }
    } // namespace Stat
} // namespace OpenKNX