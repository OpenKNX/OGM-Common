#include "OpenKNX/Led.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    void Led::init(long pin /* = -1 */, long activeOn /* = HIGH */)
    {
        // no valid pin
        if (pin < 0)
            return;

        _pin = pin;
        _activeOn = activeOn;

        pinMode(_pin, OUTPUT);
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
        if (_pin < 0) return;

        _lastMillis = millis();

        // PowerSave (Prio 1)
        if (_powerSave)
        {
            writeLed(false);
            return;
        }

        // FatalError (Prio 2)
        if (_errorMode)
        {
            writeLed(_errorEffect->value(_brightness));
            return;
        }

        // Debug (Prio 3)
#ifdef OPENKNX_HEARTBEAT
        // debug mode enable
        if (_debugMode)
        {
    #ifdef OPENKNX_HEARTBEAT_PRIO
            // Blinking until the heartbeat signal stops.
            if (!(millis() - _debugHeartbeat >= OPENKNX_HEARTBEAT))
                writeLed(_debugEffect->value(_brightness));
            else
                writeLed(false);

            return;
    #else
            // Blinks as soon as the heartbeat signal stops.
            if ((millis() - _debugHeartbeat >= OPENKNX_HEARTBEAT))
            {
                writeLed(_debugEffect->value(_brightness));
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
            if (_effectMode)
                writeLed(_effect->value(_brightness));
            else
                writeLed(true);
            return;
        }

        writeLed(false);
    }

    void Led::brightness(uint8_t brightness)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("brightness %i", _brightness);
        _brightness = brightness;
    }

    void Led::powerSave(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("powerSave %i", active);
        _powerSave = active;
    }

    void Led::forceOn(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("forceOn %i", active);
        _forceOn = active;
#ifdef OPENKNX_HEARTBEAT_PRIO
        if (_debugMode)
            _debugEffect->updateFrequency(active ? OPENKNX_HEARTBEAT_PRIO_ON_FREQ : OPENKNX_HEARTBEAT_PRIO_OFF_FREQ);
#endif
    }

    void Led::errorCode(uint8_t code /* = 0 */)
    {
        // no valid pin
        if (_pin < 0) return;

        _errorMode = false;
        if (_errorMode) delete _errorEffect;

        if (code > 0)
        {
            logTraceP("errorCode %i", code);
            _errorEffect = new LedEffects::Error(code);
            _errorMode = true;
        }
    }

    void Led::on(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("on");
        unloadEffect();
        _state = active;
    }

    void Led::pulsing(uint16_t frequency)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("pulsing (frequency %i)", frequency);
        loadEffect(new LedEffects::Pulse(frequency));
        _state = true;
    }

    void Led::blinking(uint16_t frequency)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("blinking (frequency %i)", frequency);
        loadEffect(new LedEffects::Blink(frequency));
        _state = true;
    }

    void Led::flash(uint16_t duration)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("flash (duration %i ms)", duration);
        loadEffect(new LedEffects::Flash(duration));
        _state = true;
    }

    void Led::off()
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("off");
        unloadEffect();
        _state = false;
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
        if (_pin < 0) return;

        if (brightness == _currentLedBrightness)
            return;

#ifdef ARDUINO_ARCH_ESP32
        // Special Hack for ESP32
        // Need to reset pinMode after using analogWrite
        if (_currentLedBrightness != 0 || _currentLedBrightness != 255)
            pinMode(_pin, OUTPUT);
#endif

        // logTraceP("==== > %i -> %i\n", _pin, brightness);
        if (brightness == 255)
            digitalWrite(_pin, _activeOn == HIGH ? true : false);

        else if (brightness == 0)
            digitalWrite(_pin, _activeOn == HIGH ? false : true);

        else
            analogWrite(_pin, _activeOn == HIGH ? brightness : (255 - brightness));

        _currentLedBrightness = brightness;
    }

#ifdef OPENKNX_HEARTBEAT
    void Led::debugLoop()
    {
        // Enable Debug Mode on first run
        if (!_debugMode)
        {
    #ifdef OPENKNX_HEARTBEAT_PRIO
            _debugEffect = new LedEffects::Blink(OPENKNX_HEARTBEAT_PRIO_OFF_FREQ);
    #else
            _debugEffect = new LedEffects::Blink(OPENKNX_HEARTBEAT_FREQ);
    #endif
            _debugMode = true;
        }

        _debugHeartbeat = millis();
    }
#endif

    void Led::unloadEffect()
    {
        if (_effectMode)
        {
            logTraceP("unload effect");
            _effectMode = false;
            delete _effect;
        }
    }

    void Led::loadEffect(LedEffects::Base *effect)
    {
        unloadEffect();
        logTraceP("load effect");
        _effect = effect;
        _effectMode = true;
    }

    std::string Led::logPrefix()
    {
        return openknx.logger.buildPrefix("LED", _pin);
    }

} // namespace OpenKNX