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

        // Only ONE activity-callback slot exists on the BAU and it is claimed here for
        // the KNX/NET LEDs; WidgetKnxBcu can't register its own without breaking them,
        // so it pulls these frame counters that we bump from the one callback instead.
        volatile uint32_t g_knxRxFrames = 0;
        volatile uint32_t g_knxTxFrames = 0;
        volatile bool g_knxActivityInstalled = false;

        void knxActivityCallback(uint8_t info)
        {
            if ((info >> KNX_ACTIVITYCALLBACK_NET))
                g_ipLedActivity = millis();
            else
                g_tpLedActivity = millis();

            // KNX_ACTIVITYCALLBACK_DIR bit: 0 = RECV, 1 = SEND.
            // Plain read-modify-write, not ++/+=: operator++ on a volatile is deprecated (-Wvolatile).
            if ((info >> KNX_ACTIVITYCALLBACK_DIR) & 0x01)
                g_knxTxFrames = g_knxTxFrames + 1;
            else
                g_knxRxFrames = g_knxRxFrames + 1;
        }

        void Functions::init()
        {
        }

        void Functions::setup()
        {
#ifdef DEVICE_DISPLAY_MODULE
            // WidgetKnxBcu pulls the RX/TX frame counters, so install the callback
            // unconditionally when a display is present.
            knx.bau().setActivityCallback(knxActivityCallback);
            g_knxActivityInstalled = true;
#else
            if (openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->active() ||
                openknx.ledFunctions.get(OPENKNX_LEDFUNC_NET_STATE)->active())
            {
                knx.bau().setActivityCallback(knxActivityCallback);
                g_knxActivityInstalled = true;
            }
#endif
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