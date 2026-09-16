#include "OpenKNX/Stat/RuntimeStat.h"
#include "OpenKNX/Facade.h"

// LIMITATION: Counter Overflow is possible and will result in wrong results (for long? runtimes)!
//             This is accepted, as runtime-statistics are not expected to be used in PROD environments


// TODO/Feature: Allow pause measuring for special case handling, especially long debug outputs on console
// TODO/Improvement: check integration of RuntimeStat in Module
// TODO/Feature: Allow measurement of Channels

namespace OpenKNX
{
    namespace Stat
    {
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