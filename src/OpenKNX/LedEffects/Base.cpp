#include "OpenKNX/LedEffects/Base.h"

namespace OpenKNX
{
    namespace LedEffects
    {
        bool Base::delayCheck(uint32_t micros, uint32_t duration)
        {
            return (micros - _lastMicros) >= duration;
        }

        void Base::init()
        {
            _lastMicros = 0;
        }
    } // namespace LedEffects
} // namespace OpenKNX