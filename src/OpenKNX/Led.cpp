#include "OpenKNX/Led.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Led::init(long pin /* = -1 */, long activeOn /* = HIGH */)
    {
        // no valid pin
        if (pin < 0)
            return;

        _pin = pin;
        _activeOn = activeOn;

        pinMode(pin, OUTPUT);
        digitalWrite(_pin, LOW);
    }

#ifdef __time_critical_func
    void __time_critical_func(Led::loop)()
#else
    void Led::loop()
#endif
    {
        // IMPORTANT!!! The method millis() and micros() are not incremented further in the interrupt!
        // no valid pin
        if (_pin < 0)
            return;

        _lastMillis = millis();

        // PowerSave (Prio 1)
        if (_powerSave)
        {
            writeLed(false);
            return;
        }

        // FatalError (Prio 2)
        if (_errorCode)
        {
            writeLed(_errorEffect.value());
            return;
        }

        // Debug (Prio 3)
#ifdef DEBUG_HEARTBEAT
        // debug mode enable
        if (_debugMode)
        {
#ifdef DEBUG_HEARTBEAT_PRIO
            // Blinking until the heartbeat signal stops.
            if (!(millis() - _debugHeartbeat >= DEBUG_HEARTBEAT))
            {
                writeLed(_debugEffect.value());
            }
            else
            {
                writeLed(false);
            }
            return;
#else
            // Blinks as soon as the heartbeat signal stops.
            if ((millis() - _debugHeartbeat >= DEBUG_HEARTBEAT))
            {
                writeLed(_debugEffect.value());
                return;
            }
#endif
        }
#endif

        // ForceOn (Prio 4)
        if (_forceOn)
        {
            writeLed(true);
            return;
        }

        // Normal with optional Effect (Prio 5)
        if (_state)
        {
            switch (_effect)
            {
                case LedEffect::Pulse:
                    writeLed(_pulseEffect.value(_brightness));
                    break;
                case LedEffect::Blink:
                    writeLed(_blinkEffect.value());
                    break;

                default:
                    writeLed(true);
                    break;
            }
            return;
        }

        writeLed(false);
    }

    void Led::brightness(uint8_t brightness)
    {
        // no valid pin
        if (_pin < 0)
            return;

        openknx.log(logPrefix(), "brightness %i", _brightness);
        _brightness = brightness;
    }

    void Led::powerSave(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0)
            return;

        openknx.log(logPrefix(), "powerSave %i", active);
        _powerSave = active;
    }

    void Led::forceOn(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0)
            return;

        openknx.log(logPrefix(), "forceOn %i", active);
        _forceOn = active;
#ifdef DEBUG_HEARTBEAT_PRIO
        _debugEffect.init(active ? DEBUG_HEARTBEAT_PRIO_ON_FREQ : DEBUG_HEARTBEAT_PRIO_OFF_FREQ);
#endif
    }

    void Led::errorCode(uint8_t code /* = 0 */)
    {
        // no valid pin
        if (_pin < 0)
            return;

        if (code > 0)
        {
            openknx.log(logPrefix(), "errorCode %i", code);
            _errorCode = true;
            _errorEffect.init(code);
        }
        else
        {
            _errorCode = false;
        }
    }

    void Led::on(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0)
            return;

        openknx.log(logPrefix(), "on");
        _state = active;
        _effect = LedEffect::Normal;
    }

    void Led::pulsing(uint16_t frequency)
    {
        // no valid pin
        if (_pin < 0)
            return;

        openknx.log(logPrefix(), "pulsing (frequency %i)", frequency);
        _state = true;
        _effect = LedEffect::Pulse;
        _pulseEffect.init(frequency);
    }

    void Led::blinking(uint16_t frequency)
    {
        // no valid pin
        if (_pin < 0)
            return;

        openknx.log(logPrefix(), "blinking (frequency %i)", frequency);
        _state = true;
        _effect = LedEffect::Blink;
        _blinkEffect.init(frequency);
    }

    void Led::off()
    {
        // no valid pin
        if (_pin < 0)
            return;

        openknx.log(logPrefix(), "off");
        _state = false;
        _effect = LedEffect::Normal;
    }

    /*
     * write led state based on bool
     */
    void Led::writeLed(bool state)
    {
        writeLed((uint8_t)(state ? _brightness : 0));
    }

    /*
     * write led state based on bool and _brightness
     */
    void Led::writeLed(uint8_t brightness)
    {
        // no valid pin
        if (_pin < 0)
            return;

        if (brightness == _currentLedBrightness)
            return;

        // SERIAL_DEBUG.printf("==== > %i -> %i\n", _pin, brightness);
        if (brightness == 255)
            digitalWrite(_pin, _activeOn == HIGH ? true : false);

        else if (brightness == 0)
            digitalWrite(_pin, _activeOn == HIGH ? false : true);

        else
            analogWrite(_pin, _activeOn == HIGH ? brightness : (255 - brightness));

        _currentLedBrightness = brightness;
    }

#ifdef DEBUG_HEARTBEAT
    void Led::debugLoop()
    {
        // Enable Debug Mode on first run
        if (!_debugMode)
        {

#ifdef DEBUG_HEARTBEAT_PRIO
            _debugEffect.init(DEBUG_HEARTBEAT_PRIO_OFF_FREQ);
#else
            _debugEffect.init(DEBUG_HEARTBEAT_FREQ);
#endif
            _debugMode = true;
        }
        _debugHeartbeat = millis();
    }
#endif

    std::string Led::logPrefix()
    {
        return openknx.logger.logPrefix("LED", _pin);
    }

} // namespace OpenKNX