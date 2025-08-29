#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        void Manager::init()
        {
            for(uint8_t i = 0; i < _ledCount; i++)
            {
                _leds[i]->init();
            }
            _init = true;
        }

        void Manager::loop()
        {

        }

        bool Manager::addLed(Led::Base* led, Led::LedType type)
        {
            if (led == nullptr)
                return false;
            
            _leds[_ledCount] = led;
            _ledCount++;

            if (type == LED_TYPE_PROG)
                _progLed = led;

            return true;
        }

        void __time_critical_func(Manager::timer)()
        {
            if(!_init)
                return;
            // distribute the 1ms timer evenly to all leds
            // 100Hz frequency
            uint32_t time = millis();
            for(uint8_t i = 0; i < _ledCount; i++)
            {
                //if(i%10 == time%10)
                    if (_leds[i] != nullptr)
                        _leds[i]->loop();
            }
        }

        Led::Base* Manager::getProgLed()
        {
            return _progLed;
        }

        Led::Base* Manager::getLed(uint8_t index)
        {
            if (index >= _ledCount)
                return _dummyLed;
            return _leds[index];
        }

        void Manager::powerSave(bool active)
        {
            for(uint8_t i = 0; i < _ledCount; i++)
            {
                if (_leds[i] != nullptr)
                    _leds[i]->powerSave(active);
            }
        }

        std::string Manager::logPrefix()
        {
            return "LED-Manager";
        }
    } // namespace Led
} // namespace OpenKNX