#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        void Manager::init()
        {

        }

        void Manager::loop()
        {

        }

        void __time_critical_func(Manager::timer)()
        {
            _progLed->loop();
        }

        Led::Base* Manager::getProgLed()
        {
            return &_progLed;
        }

        void Manager::powerSave(bool active)
        {
            _progLed->powerSave(active);
        }

        std::string Manager::logPrefix()
        {
            return "LED-Manager";
        }
    } // namespace Led
} // namespace OpenKNX