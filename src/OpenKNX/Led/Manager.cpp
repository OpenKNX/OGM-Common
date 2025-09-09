#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        Manager::Manager()
        {
        }

        void Manager::init()
        {
#ifndef OPENKNX_LED_NO_AUTOCONF
    #ifdef OPENKNX_SERIALLED_ENABLE
            addLed(new OpenKNX::Led::Serial(PROG_LED_PIN, OPENKNX_SERIALLED_PIN, PROG_LED_COLOR), OpenKNX::Led::LED_TYPE_PROG);
        #ifdef INFO1_LED_PIN
            addLed(new OpenKNX::Led::Serial(INFO1_LED_PIN, OPENKNX_SERIALLED_PIN, INFO1_LED_COLOR), OpenKNX::Led::LED_TYPE_INFO1);
        #endif
        #ifdef INFO2_LED_PIN
            addLed(new OpenKNX::Led::Serial(INFO2_LED_PIN, OPENKNX_SERIALLED_PIN, INFO2_LED_COLOR), OpenKNX::Led::LED_TYPE_INFO2);
        #endif
        #ifdef INFO3_LED_PIN
            addLed(new OpenKNX::Led::Serial(INFO3_LED_PIN, OPENKNX_SERIALLED_PIN, INFO3_LED_COLOR), OpenKNX::Led::LED_TYPE_INFO3);
        #endif
    #else
            addLed(new OpenKNX::Led::GPIO(PROG_LED_PIN, PROG_LED_PIN_ACTIVE_ON), OpenKNX::Led::LED_TYPE_PROG);
        #ifdef INFO1_LED_PIN
            addLed(new OpenKNX::Led::GPIO(INFO1_LED_PIN, INFO1_LED_PIN_ACTIVE_ON), OpenKNX::Led::LED_TYPE_INFO1);
        #endif
        #ifdef INFO2_LED_PIN
            addLed(new OpenKNX::Led::GPIO(INFO2_LED_PIN, INFO2_LED_PIN_ACTIVE_ON), OpenKNX::Led::LED_TYPE_INFO2);
        #endif
        #ifdef INFO3_LED_PIN
            addLed(new OpenKNX::Led::GPIO(INFO3_LED_PIN, INFO3_LED_PIN_ACTIVE_ON), OpenKNX::Led::LED_TYPE_INFO3);
        #endif
    #endif
#endif
            logInfo("LED", "Init LED Manager with %d Leds", _ledCount);
#ifdef OPENKNX_SERIALLED_ENABLE
            if(_serialLedManager)
                _serialLedManager->init(_serialLedCount);
#endif
            for(uint8_t i = 0; i < _ledCount; i++)
            {
                _leds[i]->init();
            }
            _init = true;
        }

        void Manager::loop()
        {

        }

        uint8_t Manager::addLed(Led::Base* led, Led::LedType type)
        {
            if (led == nullptr || _init)
                return 0xFF;
            
            _leds[_ledCount] = led;
            _ledCount++;

            _specialLeds[type] = led;

            return _ledCount - 1;
        }
        
#ifdef OPENKNX_SERIALLED_ENABLE
        uint8_t Manager::addLed(Led::Serial* led, Led::LedType type)
        {
            if (led == nullptr || _init)
                return 0xFF;
            
            led->setManager(getSerialLedManager(led->getPin()));
            if(led->getAddr() >= _serialLedCount)
                _serialLedCount = led->getAddr()+1;
            
            _leds[_ledCount] = led;
            _ledCount++;

            _specialLeds[type] = led;

            return _ledCount - 1;
        }
        
        Led::SerialLedManager* Manager::getSerialLedManager(long pin)
        {
            if(_serialLedManager == nullptr)
            {
                _serialLedManager = new Led::SerialLedManager(pin);
            }
            else if(_serialLedManager->getPin() != pin)
            {
                logErrorP("", "Only one Serial LED Manager supported!");
                return nullptr;
            }
            return _serialLedManager;
        }
#endif

        void __time_critical_func(Manager::timer)(bool distribute)
        {
            if(!_init)
                return;
            if(!distribute)
            {
                // distribute the 1ms timer evenly to all leds
                // 100Hz frequency
                uint32_t time = millis();
                for(uint8_t i = 0; i < _ledCount; i++)
                {
                    if(i%10 == time%10)
                        if (_leds[i] != nullptr)
                            _leds[i]->loop();
                }
#ifdef OPENKNX_SERIALLED_ENABLE
                if(_serialLedManager && time % 10) // 100Hz frequency
                    _serialLedManager->writeLeds();
#endif
            }
            else
            {
                for(uint8_t i = 0; i < _ledCount; i++)
                {
                    if (_leds[i] != nullptr)
                        _leds[i]->loop();
                }
#ifdef OPENKNX_SERIALLED_ENABLE
                if(_serialLedManager)
                    _serialLedManager->writeLeds();
#endif
            }
        }

        Led::Base* Manager::getProgLed()
        {
            return getLed(LED_TYPE_PROG);
        }

        Led::Base* Manager::getLed(uint8_t index)
        {
            if (index >= _ledCount)
                return _dummyLed;
            return _leds[index];
        }

        Led::Base* Manager::getLed(LedType type)
        {
            if (type >= LED_TYPE_MAX)
                return _dummyLed;
            return _specialLeds[type];
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