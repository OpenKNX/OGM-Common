#pragma once

#include "DurationStatistic.h"
#include <Arduino.h>
#include <string>

#ifdef OPENKNX_RUNTIME_STAT
    #define RUNTIME_MEASURE_BEGIN(X) (X).measureTimeBegin();
    #define RUNTIME_MEASURE_END(X) (X).measureTimeEnd();
    #define RUNTIME_MEASURE_END_WARN(X, WARN_US, WARN_LABEL) {\
			const uint32_t duration_us = (X).measureTimeEndReturnDuration();\
            if (duration_us > WARN_US) {\
                openknx.logger.logWithPrefixAndValues(WARN_LABEL, "Warning: RuntimeStat duration %d > %d µs", duration_us, WARN_US);\
            }\
    }
    
#else
    #define RUNTIME_MEASURE_BEGIN(X)
    #define RUNTIME_MEASURE_END(X)
    #define RUNTIME_MEASURE_END_WARN(X, WARN_US, WARN_LABEL)
#endif

#define OPENKNX_RUNTIME_STAT_BUCKETN1 (OPENKNX_RUNTIME_STAT_BUCKETN-1)

namespace OpenKNX
{
    namespace Stat
    {
        class RuntimeStat
        {
          private:
            uint32_t _begin_us = 0;
            uint32_t _end_us = 0;

            DurationStatistic _run = DurationStatistic();
            DurationStatistic _wait = DurationStatistic();

          public:
            static void showStatHeader();

            inline void measureTimeBegin()
            {
                _begin_us = micros();

                // measure waiting-time between two loops
                if (_end_us > 0)
                {
                    _wait.measure(_begin_us - _end_us);
                }
            }

            inline void measureTimeEnd()
            {
                // store end only once at the beginning, as getting the time twice might increase error
                _end_us = micros();

                _run.measure(_end_us - _begin_us);
            }

            inline uint32_t measureTimeEndReturnDuration()
            {
                // store end only once at the beginning, as getting the time twice might increase error
                _end_us = micros();

                const uint32_t duration = _end_us - _begin_us;
                _run.measure(duration);
                return duration;
            }

            void showStat(std::string label, const uint8_t core = 0, const bool stat = true, const bool hist = false);
        };
    } // namespace Stat
} // namespace OpenKNX