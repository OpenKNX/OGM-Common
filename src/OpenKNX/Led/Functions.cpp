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
                /*
                if(openknxNetwork.established())
                {
                    if(_ipLedState != 1)
                    {
                        #ifdef OPENKNX_SERIALLED_ENABLE
                        openknx.info2Led.setColor(OPENKNX_SERIALLED_COLOR_GREEN);
                        #endif
                        openknx.info2Led.activity(_ipLedActivity, true);
                        _ipLedState = 1;
                    }
                }
                else if(openknxNetwork.connected())
                {
                    if(_ipLedState != 2)
                    {
                        #ifdef OPENKNX_SERIALLED_ENABLE
                        openknx.info2Led.setColor(OPENKNX_SERIALLED_COLOR_YELLOW);
                        openknx.info2Led.on();
                        #else
                        openknx.info2Led.off();
                        #endif

                        _ipLedState = 2;
                    }
                }
                else
                {
                    if(_ipLedState != 3)
                    {
                        #ifdef OPENKNX_SERIALLED_ENABLE
                        openknx.info2Led.setColor(OPENKNX_SERIALLED_COLOR_RED);
                        openknx.info2Led.on();
                        #else
                        openknx.info2Led.off();
                        #endif
                        _ipLedState = 3;
                    }
                }
                */
#if MASK_VERSION == 0x091A
                if (knx.bau().getSecondaryDataLinkLayer()->isConnected())
#else
                if (knx.bau().getDataLinkLayer()->isConnected())
#endif
                {
                    if (_tpLedState != 1)
                    {
                        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->setColor(Led::Color::Green);
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
                        openknx.ledFunctions.get(OPENKNX_LEDFUNC_BASE_KNX)->setColor(Led::Color::Red);
                        _tpLedState = 3;
                    }
                }

                _leds = millis();
            }
        }
    } // namespace Led
} // namespace OpenKNX