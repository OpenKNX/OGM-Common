#pragma once
#include "OpenKNX/LedEffects/Base.h"

#ifndef OPENKNX_LEDEFFECT_ACTIVITY_DURATION
    #define OPENKNX_LEDEFFECT_ACTIVITY_DURATION 50
#endif

#ifndef OPENKNX_LEDEFFECT_ACTIVITY_PAUSE
    #define OPENKNX_LEDEFFECT_ACTIVITY_PAUSE 20
#endif

namespace OpenKNX
{
    namespace LedEffects
    {
        class Activity : public Base
        {
          protected:
            uint32_t &_lastActivity;
            bool _inverted = false;

          public:
            Activity(uint32_t &lastActivity, bool inverted = false) : _lastActivity(lastActivity), _inverted(inverted){};
            ~Activity(){};
            uint8_t value(uint8_t maxValue) override;
        };
    } // namespace LedEffects
} // namespace OpenKNX