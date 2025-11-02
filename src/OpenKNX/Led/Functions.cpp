#include "OpenKNX/Led/Functions.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        Functions::Functions()
        {
        }

        void ledFunctionsActivityCallback(uint8_t info)
        {
        }

        void Functions::init()
        {
            knx.bau().setActivityCallback(ledFunctionsActivityCallback);
        }

        void Functions::setup()
        {
            openknx.ledFunctions.getActive(9);
        }

        void Functions::loop()
        {
            knx.progMode();
        }
    } // namespace Led
} // namespace OpenKNX