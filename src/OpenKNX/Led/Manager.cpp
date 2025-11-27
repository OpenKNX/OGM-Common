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
    #ifdef LED_INIT
            LED_INIT();
    #else
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
#endif
            logInfoP("Init %d Leds", _leds.size());
#ifdef OPENKNX_SERIALLED_ENABLE
            if (_serialLedManager)
                _serialLedManager->init(_serialLedCount);
#endif

            for (const auto& pair : _leds)
                pair.second->init();

            _init = true;
        }

        void Manager::loop()
        {
        }

        void Manager::addLed(Led::Base* led, uint8_t identifier)
        {
            if (led == nullptr)
            {
                logErrorP("Cannot add LED after init or led is null");
                return;
            }
            led->setIdentifier(identifier);
            _leds[identifier] = led;
        }

#ifdef OPENKNX_SERIALLED_ENABLE
        void Manager::addLed(Led::Serial* led, uint8_t identifier)
        {
            if (led == nullptr || _init)
            {
                logErrorP("Cannot add LED after init or led is null");
                return;
            }

            led->setManager(getSerialLedManager(led->getPin()));
            if (led->getAddr() >= _serialLedCount)
                _serialLedCount = led->getAddr() + 1;
            led->setIdentifier(identifier);
            _leds[identifier] = led;
        }

        Led::SerialLedManager* Manager::getSerialLedManager(long pin)
        {
            if (_serialLedManager == nullptr)
            {
                _serialLedManager = new Led::SerialLedManager(pin);
            }
            else if (_serialLedManager->getPin() != pin)
            {
                logErrorP("Only one Serial LED Manager supported!");
                return nullptr;
            }
            return _serialLedManager;
        }
#endif

        void __time_critical_func(Manager::timer)(bool distribute /* = true */)
        {
            if (!_init)
                return;
            
            uint32_t time = millis();
            if (distribute) // default
            {
                // distribute the 1ms timer evenly to all leds
                // 100Hz frequency
                uint8_t i = 0;
                for (const auto& pair : _leds)
                {
                    if (i % 10 == time % 10)
                        pair.second->loop();
                    i++;
                }
#ifdef OPENKNX_SERIALLED_ENABLE
                if (_serialLedManager && time % 10) // 100Hz frequency
                    _serialLedManager->writeLeds();
#endif
            }
            else
            {
                if (!(time % 10)) // 100Hz frequency
                {
                    for (const auto& pair : _leds)
                        pair.second->loop();

#ifdef OPENKNX_SERIALLED_ENABLE
                    if (_serialLedManager)
                        _serialLedManager->writeLeds();
#endif
                }
            }
        }

        Led::Base* Manager::getProgLed()
        {
            return getLed(LED_TYPE_PROG);
        }

        Led::Base* Manager::getLed(uint8_t identifier)
        {
            return _leds.find(identifier) != _leds.end() ? _leds[identifier] : _dummyLed;
        }

        void Manager::powerSave(bool active)
        {
            for (const auto& pair : _leds)
                pair.second->powerSave(active);
        }

        std::string Manager::logPrefix()
        {
            return "LED-Manager";
        }
    } // namespace Led
} // namespace OpenKNX