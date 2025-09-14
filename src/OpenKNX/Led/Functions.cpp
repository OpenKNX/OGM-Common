#include "OpenKNX/Led/Functions.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        Functions::Functions()
        {
        }

        void Functions::setup()
        {
            // setup the info1-3 led according to knx parameters
            if(ParamBASE_Info1LedType)
            {
                if(!AssignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO1), ParamBASE_Info1LedFunc))
                {
                    logDebug("Led::Functions", "Could not assign Info1 LED to function %d", ParamBASE_Info1LedFunc);
                }
            }
            if(ParamBASE_Info2LedType)
            {
                if(!AssignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO2), ParamBASE_Info2LedFunc))
                {
                    logDebug("Led::Functions", "Could not assign Info2 LED to function %d", ParamBASE_Info2LedFunc);
                }
            }
            if(ParamBASE_Info3LedType)
            {
                if(!AssignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO3), ParamBASE_Info3LedFunc))
                {
                    logDebug("Led::Functions", "Could not assign Info3 LED to function %d", ParamBASE_Info3LedFunc);
                }
            }
        }

        void Functions::loop()
        {
            // maybe do some periodic led stuff here
            // not needed currently
        }

        void Functions::processInputKo(GroupObject& ko)
        {
            // process info1-3 led input ko
            // no Ko control in common currently
        }

        bool Functions::RegisterLedFunction(uint32_t functionId, Led::Base** led)
        {
            // add id/pointer to list
            return false;
        }

        bool Functions::AssignLed2Function(Led::Base* led, uint32_t functionId)
        {
            // assign the led pointer to the pointer that was registered with the functionId
            return false;
        }
    }
}