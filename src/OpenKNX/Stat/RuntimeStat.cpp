#include "OpenKNX/Stat/RuntimeStat.h"
#include "OpenKNX/Facade.h"

// TODO/LIMITATION: Counter Overflow ist possible and will result in wrong results (for long? runtimes)!

// TODO/Feature: Allow pause measuring for special case handling
// TODO/Feature: add measuring for core1
// TODO/Improvement: check integration of RuntimeStat in Module
// TODO/Feature: Allow measurement of Channels

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
            openknx.logger.logWithPrefixAndValues("RuntimeStat", "@ type  param unit    value_run   value_wait");
        }

        void RuntimeStat::showStat(std::string label, const uint8_t core /*= 0*/, const bool stat /*= false*/, const bool hist /*= false*/)
        {
            if (stat)
            {
                openknx.logger.logWithPrefixAndValues(label, "%d stat  count    # %12d %12d", core, _run._count, _wait._count);
                openknx.logger.logWithPrefixAndValues(label, "%d stat    sum   ms %12d %12d", core, _run.sum_ms(), _wait.sum_ms());
                openknx.logger.logWithPrefixAndValues(label, "%d stat    min   us %12d %12d", core, _run.durationMin_us, _wait.durationMin_us);
                openknx.logger.logWithPrefixAndValues(label, "%d stat    avg   us %12d %12d", core, _run.avg_us(), _wait.avg_us());
                openknx.logger.logWithPrefixAndValues(label, "%d stat   ~med   us %12d %12d", core, _run.estimateMedian_us(), _wait.estimateMedian_us());
                openknx.logger.logWithPrefixAndValues(label, "%d stat    max   us %12d %12d", core, _run.durationMax_us, _wait.durationMax_us);
            }
            if (hist)
            {
                for (size_t i = 0; i < OPENKNX_RUNTIME_STAT_BUCKETN1; i++)
                {
                    openknx.logger.logWithPrefixAndValues(label, "%d hist %6d  #<= %12d %12d", core, DurationStatistic::getHistBucketUpper_us(i), _run.getHistBucket(i), _wait.getHistBucket(i));
                }
                openknx.logger.logWithPrefixAndValues(label, "%d hist INFu32  #<= %12d %12d", core, _run.getHistBucket(OPENKNX_RUNTIME_STAT_BUCKETN1), _wait.getHistBucket(OPENKNX_RUNTIME_STAT_BUCKETN1));
            }
        }
    } // namespace Stat
} // namespace OpenKNX