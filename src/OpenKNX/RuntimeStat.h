#pragma once

#include <Arduino.h>

namespace OpenKNX
{
    class RuntimeStat
    {
      private:
        // TODO use constant const static size_t _timeN = 7;
        const static uint32_t _timeRangeMax[16];
        uint32_t _begin_us = 0;
        uint32_t _count = 0;
        uint32_t _countRange[16] = {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
        };
        
      protected:

      public:
        void measureTimeBegin();
        void measureTimeEnd();
        void showStat();
    };
} // namespace OpenKNX