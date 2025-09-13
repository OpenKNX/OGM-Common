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
        }

        void Functions::loop()
        {
            // maybe do some periodic led stuff here
        }

        void Functions::processInputKo(GroupObject& ko)
        {
            // process info1-3 led input ko
        }

        bool Functions::RegisterLedFunction(uint32_t functionId, Led::Base* led)
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