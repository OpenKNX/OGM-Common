#include "OpenKNX/Led/FunctionManager.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        FunctionManager::FunctionManager()
        {
        }

        void FunctionManager::init()
        {
            // assign the LED_TYPE_PROG to Prog-LedFunction
            assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_PROG), OPENKNX_LEDFUNC_BASE_PROG);
        }

        void FunctionManager::setup()
        {
            if (useDefaultFunction())
            {
                logDebugP("useDefaultFunction %i", ParamBASE_DefaultLedFunc);
#ifdef OPENKNX_INFOLED1_DEFAULT
                assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO1), OPENKNX_INFOLED1_DEFAULT);
#else
                assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO1), OPENKNX_LEDFUNC_BASE_STATE);
#endif
#ifdef OPENKNX_INFOLED2_DEFAULT
                assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO2), OPENKNX_INFOLED2_DEFAULT);
#endif
#ifdef OPENKNX_INFOLED3_DEFAULT
                assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO3), OPENKNX_INFOLED3_DEFAULT);
#endif
            }
            else if (knx.configured())
            {
                // setup info1-3 led according to knx parameters
                logDebugP("setup info1-3 by parameters");
                assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO1), ParamBASE_Info1LedFunc);
                assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO2), ParamBASE_Info2LedFunc);
                assignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO3), ParamBASE_Info3LedFunc);
            }
        }

        bool FunctionManager::useDefaultFunction()
        {
            if (!knx.configured()) return true;

#ifdef ParamBASE_DefaultLedFunc
            if (ParamBASE_DefaultLedFunc) return true;
#endif

            return false;
        }

        void FunctionManager::assignLed2Function(Led::Base* led, uint32_t functionId)
        {
            if (functionId == 0) return;

            FunctionGroup* fg = get(functionId);
            fg->addLed(led);
            logDebugP("assignLed2Function led pointer value: %p to functionId %u", led, functionId);
        }

        std::string FunctionManager::logPrefix()
        {
            return "LED-Functions";
        }

        FunctionGroup* FunctionManager::get(uint32_t functionId)
        {
            auto it = _functionGroups.find(functionId);
            if (it == _functionGroups.end())
            {
                // construct a new FunctionGroup with the correct id
                auto [insertIt, _] = _functionGroups.emplace(functionId, FunctionGroup(functionId));
                it = insertIt;
            }

            return &it->second;
        }

        bool FunctionGroup::active()
        {
            return _active;
        }

        void FunctionGroup::addLed(Led::Base* led)
        {
            _active = true;
            _leds.push_back(led);
        }

        void FunctionGroup::on(bool state, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == Capability::ALL ||
                    (capability == Capability::MONOCHROME && !led->isColor()) ||
                    (capability == Capability::COLOR && led->isColor()))
                {
                    led->on(state);
                }
            }
        }

        void FunctionGroup::on(Capability capability)
        {
            on(true, capability);
        }

        void FunctionGroup::off(Capability capability)
        {
            on(false, capability);
        }

        void FunctionGroup::color(uint8_t r, uint8_t g, uint8_t b)
        {
            for (auto led : _leds)
            {
                if (led->isColor())
                {
                    ((RGB*)led)->color(r, g, b);
                }
            }
        }

        void FunctionGroup::color(uint32_t rgb)
        {
            for (auto led : _leds)
            {
                if (led->isColor())
                {
                    ((RGB*)led)->color(rgb);
                }
            }
        }

        void FunctionGroup::color(Color value)
        {
            color(static_cast<uint32_t>(value));
        }

        void FunctionGroup::pulsing(uint16_t duration, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == Capability::ALL ||
                    (capability == Capability::MONOCHROME && !led->isColor()) ||
                    (capability == Capability::COLOR && led->isColor()))
                {
                    if (led->isDimmable())
                        led->pulsing(duration);
                    else
                        led->on();
                }
            }
        }

        void FunctionGroup::blinking(uint16_t frequency, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == Capability::ALL ||
                    (capability == Capability::MONOCHROME && !led->isColor()) ||
                    (capability == Capability::COLOR && led->isColor()))
                {
                    led->blinking(frequency);
                }
            }
        }

        void FunctionGroup::flash(uint16_t duration, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == Capability::ALL ||
                    (capability == Capability::MONOCHROME && !led->isColor()) ||
                    (capability == Capability::COLOR && led->isColor()))
                {
                    led->flash(duration);
                }
            }
        }

        void FunctionGroup::forceOn(bool state)
        {
            for (auto led : _leds)
            {
                led->forceOn(state);
            }
        }

#ifdef OPENKNX_HEARTBEAT
        void FunctionGroup::debugLoop()
        {
            for (auto led : _leds)
            {
                led->debugLoop();
            }
        }
#endif

        void FunctionGroup::powerSave(bool active)
        {
            for (auto led : _leds)
            {
                led->powerSave(active);
            }
        }

        void FunctionGroup::errorCode(uint8_t code)
        {
            for (auto led : _leds)
            {
                led->errorCode(code);
            }
        }

        void FunctionGroup::activity(uint32_t& lastActivity, bool inverted, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == Capability::ALL ||
                    (capability == Capability::MONOCHROME && !led->isColor()) ||
                    (capability == Capability::COLOR && led->isColor()))
                    led->activity(lastActivity, inverted);
            }
        }

        std::string FunctionGroup::logPrefix()
        {
            return "LED-FunctionGroup";
        }
    } // namespace Led
} // namespace OpenKNX