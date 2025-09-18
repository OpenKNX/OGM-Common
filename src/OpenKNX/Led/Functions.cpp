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
            AssignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO1), ParamBASE_Info1LedFunc);
            AssignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO2), ParamBASE_Info2LedFunc);
            AssignLed2Function(openknx.leds.getLed(Led::LedType::LED_TYPE_INFO3), ParamBASE_Info3LedFunc);
        }

        void Functions::AssignLed2Function(Led::Base* led, uint32_t functionId)
        {
            FunctionGroup* fg = get(functionId);
            fg->addLed(led);
            logDebugP("AssignLed2Function led pointer value: %p to functionId %u", led, functionId);
        }

        std::string Functions::logPrefix()
        {
            return "LED-Functions";
        }

        FunctionGroup* Functions::get(uint32_t functionId)
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


        void FunctionGroup::addLed(Led::Base* led)
        {
            _leds.push_back(led);
        }

        void FunctionGroup::on(bool state, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == ALL ||
                    (capability == MONOCHROME && !led->isRGB()) ||
                    (capability == COLOR && led->isRGB()))
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

        void FunctionGroup::setColor(uint8_t r, uint8_t g, uint8_t b)
        {
            for (auto led : _leds)
            {
                if (led->isRGB())
                {
                    ((RGB*)led)->setColor(r, g, b);
                }
            }
        }

        void FunctionGroup::setColor(uint32_t rgb)
        {
            for (auto led : _leds)
            {
                if (led->isRGB())
                {
                    ((RGB*)led)->setColor(rgb);
                }
            }
        }

        void FunctionGroup::pulsing(uint16_t duration, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == ALL ||
                    (capability == MONOCHROME && !led->isRGB()) ||
                    (capability == COLOR && led->isRGB()))
                {
                    led->pulsing(duration);
                }
            }
        }

        void FunctionGroup::blinking(uint16_t frequency, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == ALL ||
                    (capability == MONOCHROME && !led->isRGB()) ||
                    (capability == COLOR && led->isRGB()))
                {
                    led->blinking(frequency);
                }
            }
        }

        void FunctionGroup::flash(uint16_t duration, Capability capability)
        {
            for (auto led : _leds)
            {
                if (capability == ALL ||
                    (capability == MONOCHROME && !led->isRGB()) ||
                    (capability == COLOR && led->isRGB()))
                {
                    led->flash(duration);
                }
            }
        }

        std::string FunctionGroup::logPrefix()
        {
            return "LED-FunctionGroup";
        }
    }
}