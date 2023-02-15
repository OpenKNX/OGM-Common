#include "OpenKNX/Led.h"
#include "OpenKNX/Common.h"

namespace OpenKNX
{
    void Led::init(long pin /* = -1 */, long activeOn /* = HIGH */)
    {
        // not configured
        if (_pin == -1)
            return;

        _pin = pin;
        _activeOn = activeOn;

        pinMode(pin, OUTPUT);
        digitalWrite(_pin, LOW);
    }

    void Led::loop()
    {
        loop(micros());
    }

    void Led::loop(uint32_t micros)
    {
        if (_pin == -1)
            return;

        _lastMicros = micros;

        // Prio 1
        if (_powerSave)
        {
            writeLed(false);
            return;
        }

        // Prio 2
        if (_forceOn)
        {
            writeLed(true);
            return;
        }

        // Prio 3
        // FatalError

        // Prio 4 OM with optional Effect
        if (_state)
        {
            if (_effect == LedEffect::Pulse)
            {
                writeLed(_pulseEffect.value(micros, _brightness));
                return;
            }
            else if (_state && _effect == LedEffect::Blink)
            {
                writeLed(_blinkEffect.value(micros));
                return;
            }

            // No effect
            writeLed(true);
            return;
        }

        // Prio 5 (Debug)
#ifdef OPENKNX_DEBUG_LOOP
        // debug mode enable
        if (_debugMode)
        {
            // timer expired - turn off
            if (!_debugLast && (micros - _debugMicros) >= 10000)
            {
                writeLed(false);
                _debugMode = false;
                return;
            }

            // loop called - reset timer
            if (_debugLast)
            {
                _debugMicros = micros;
                _debugLast = false;
            }

            writeLed(_debugEffect.value(micros));
            return;
        }
#endif

        // Last option if nothing before matched -> off
        writeLed(false);
    }

    void Led::brightness(uint8_t brightness)
    {
        _brightness = brightness;
    }

    void Led::powerSave(bool active /* = true */)
    {
        _powerSave = active;
    }

    void Led::forceOn(bool active /* = true */)
    {
        _forceOn = active;
    }

    void Led::errorCode(uint8_t code /* = 0 */)
    {
        _errorCode = code;
    }

    void Led::on()
    {
        _state = true;
        _effect = LedEffect::Normal;
    }

    void Led::pulsing(uint16_t frequency)
    {
        _state = true;
        _effect = LedEffect::Pulse;
        _pulseEffect.init(frequency);
    }

    void Led::blinking(uint16_t frequency)
    {
        _state = true;
        _effect = LedEffect::Blink;
        _blinkEffect.init(frequency);
    }

    void Led::off()
    {
        _state = false;
        _effect = LedEffect::Normal;
    }

    /*
     * write led state based on boo
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

#ifdef OPENKNX_DEBUG_LOOP
    void Led::debugLoop()
    {
        // Enable Debug Mode
        if (!_debugMode)
        {
            _debugMode = true;
            _debugEffect.init(1000);
        }
        _debugLast = true;
    }
#endif

} // namespace OpenKNX