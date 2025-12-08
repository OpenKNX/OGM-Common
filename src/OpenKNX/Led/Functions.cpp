#include "OpenKNX/Led/Functions.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        Functions::Functions()
        {
        }

        uint32_t g_tpLedActivity = 0;
        uint32_t g_ipLedActivity = 0;
        void knxActivityCallback(uint8_t info)
        {
            if ((info >> KNX_ACTIVITYCALLBACK_NET))
                g_ipLedActivity = millis();
            else
                g_tpLedActivity = millis();
        }

        void Functions::init()
        {
        }

        void Functions::setup()
        {
            if (openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->active() ||
                openknx.ledFunctions.get(OPENKNX_LEDFUNC_NET_STATE)->active())
            {
                knx.bau().setActivityCallback(knxActivityCallback);
            }
        }

        void Functions::loop()
        {
            if (delayCheck(_leds, 100))
            {
#if MASK_VERSION == 0x091A
                if (knx.bau().getSecondaryDataLinkLayer()->isConnected())
#elif MASK_VERSION == 0x07B0
                if (knx.bau().getDataLinkLayer()->isConnected())
#else // e.g. 57B0, KNX-IP-Only, no Busstatus availible
                if (false)
#endif
                {
                    if (_tpLedState != 1)
                    {
                        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->color(Led::Color::Green);
                        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->activity(g_tpLedActivity, true);
                        _tpLedState = 1;
                    }
                }
                else
                {
                    if (_tpLedState != 3)
                    {
                        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->off(Capability::MONOCHROME);
                        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->on(Capability::COLOR);
                        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->color(Led::Color::Red);
                        _tpLedState = 3;
                    }
                }

                _leds = millis();
            }
        }
    } // namespace Led
} // namespace OpenKNX