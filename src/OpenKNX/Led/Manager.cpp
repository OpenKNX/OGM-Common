#include "OpenKNX/Led/Manager.h"
#include "OpenKNX/Facade.h"
#include "OpenKNX/TimerInterrupt.h" // For OPENKNX_INTERRUPT_TIMER_MS

namespace OpenKNX
{
    namespace Led
    {
        volatile uint8_t Manager::_pwmCycle = 0;
        // Soft-PWM resolution; keep 10 (fewer steps drops the low end, e.g. `dim 20` rounds to OFF).
        // Runtime-tunable via `leds pwm steps <n>`.
        volatile uint8_t Manager::_pwmSteps = 10;
        volatile uint16_t Manager::_timerUpdateHz = 1000 / OPENKNX_INTERRUPT_TIMER_MS;

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

            _pwmCycle = (_pwmCycle + 1) % _pwmSteps;

            for (const auto& pair : _leds)
                pair.second->loop();

#ifdef OPENKNX_SERIALLED_ENABLE
            if (_serialLedManager)
                _serialLedManager->writeLeds();
#endif
        }

        // Flush all pending I2C LED writes; the PCA95xx leaf takes the shared Wire1 mutex, so do
        // not lock here. ESP32: driven at ~50 Hz from the esp_timer flush task for smooth dimming.
        void Manager::flushI2C()
        {
            if (!_init)
                return;

            for (const auto& pair : _leds)
            {
                GPIO* gpio = pair.second->asGPIO();
                if (gpio)
                    gpio->flushPendingI2C();
            }
        }

        void Manager::loop()
        {
            if (!_init)
                return;

            // RP2040 only: flush here. On ESP32 the flush task owns it at tick rate.
#ifndef ARDUINO_ARCH_ESP32
            flushI2C();
#endif

            // Self-heal (500 ms): re-assert each I2C LED pin direction+level; recovers a PCA9557
            // CONFIG-register glitch on the display-shared Wire1 without a reboot. Do not go below ~300 ms.
            static uint32_t lastReassertMs = 0;
            const uint32_t now = millis();
            if (now - lastReassertMs >= 500)
            {
                lastReassertMs = now;
                for (const auto& pair : _leds)
                {
                    GPIO* gpio = pair.second->asGPIO();
                    if (gpio)
                        gpio->reassertI2C();
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