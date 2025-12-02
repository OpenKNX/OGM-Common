#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/TimerInterrupt.h"  // For OPENKNX_INTERRUPT_TIMER_MS

namespace OpenKNX
{
    namespace Led
    {
        // Global PWM configuration (updated every timer interrupt)
        volatile uint8_t Manager::_pwmCycle = 0;
        volatile uint8_t Manager::_pwmSteps = 10;  // Default: 10 steps
        volatile uint16_t Manager::_timerUpdateHz = 1000 / OPENKNX_INTERRUPT_TIMER_MS; // Calculate from timer interval
        
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

        void Manager::addLed(Led::Base* led, uint8_t identifier)
        {
            if (led == nullptr)
            {
                logDebugP("Cannot add LED: led is null");
                return;
            }
            if (_init)
            {
                logErrorP("Cannot add LED after init");
                return;
            }
            if (_leds.find(identifier) != _leds.end())
            {
                logDebugP("Cannot add LED: identifier %d already in use", identifier);
                return;
            }
            led->setIdentifier(identifier);
            _leds[identifier] = led;
        }

#ifdef OPENKNX_SERIALLED_ENABLE
        void Manager::addLed(Led::Serial* led, uint8_t identifier)
        {
            if (led == nullptr)
            {
                logDebugP("Cannot add Serial LED: led is null");
                return;
            }
            if (_init)
            {
                logErrorP("Cannot add LED after init");
                return;
            }
            if (_leds.find(identifier) != _leds.end())
            {
                logDebugP("Cannot add LED: identifier %d already in use", identifier);
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

        void __time_critical_func(Manager::timer)(bool doNotCheckMillis /*= false*/)
        {
            if (!_init)
                return;

            // Increment PWM cycle counter (configurable steps)
            _pwmCycle = (_pwmCycle + 1) % _pwmSteps;

            // LED effects every 10ms (100Hz)
            for (const auto& pair : _leds)
                pair.second->loop();

#ifdef OPENKNX_SERIALLED_ENABLE
            // Serial LEDs every 10ms (100Hz)
            if (_serialLedManager)
                _serialLedManager->writeLeds();
#endif
        }

        // Main loop - flush pending I2C LED updates
        void Manager::loop()
        {
            if (!_init)
                return;

#if defined(OPENKNX_I2C_USE_PENDING_PATTERN) || defined(OPENKNX_I2C_USE_SPINLOCK) 
            // Flush all pending I2C writes from main loop
            // - PENDING_PATTERN: All I2C writes happen here
            // - SPINLOCK: Only failed ISR writes are retried here with blocking lock
            for (const auto& pair : _leds)
            {
                GPIO* gpio = dynamic_cast<GPIO*>(pair.second);
                if (gpio)
                    gpio->flushPendingI2C();
            }
#endif

#if defined(OPENKNX_I2C_USE_ASYNC_QUEUE)
            // ASYNC_QUEUE: Process queue is handled in Common.cpp loop()
            // No action needed here - LEDs are already queued by writeLed()
            // This comment ensures loop() still runs for consistency
#endif
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