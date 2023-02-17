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
        // IMPORTANT!!! The method millis() and micros() are not incremented further in the interrupt!
        if (_pin == -1)
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
#if defined(DEBUG_HEARTBEAT_PRIO)
        // debug mode enable
        if (_debugMode)
        {
// heartbeat expire -> blink
            if (!(millis() - _debugHeartbeat >= DEBUG_HEARTBEAT_PRIO))
            {
                writeLed(_debugEffect.value());
            }
            else
            {

                writeLed(false);
            }
            return;
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

        // Debug (Prio 6)
#if defined(DEBUG_HEARTBEAT) && !defined(DEBUG_HEARTBEAT_PRIO)
        // debug mode enable
        if (_debugMode)
        {
// heartbeat expire -> blink
            if ((millis() - _debugHeartbeat >= DEBUG_HEARTBEAT))
            {
                writeLed(_debugEffect.value());
                return;
            }
        }
#endif

        // OFF (Prio 7)
        writeLed(false);
    }

    void Led::brightness(uint8_t brightness)
    {
        openknx.logger.log(LogLevel::Trace, "LED", "brightness %i", _brightness);
        _brightness = brightness;
    }

    void Led::powerSave(bool active /* = true */)
    {
        openknx.logger.log(LogLevel::Trace, "LED", "powerSave %i", active);
        _powerSave = active;
    }

    void Led::forceOn(bool active /* = true */)
    {
        openknx.logger.log(LogLevel::Trace, "LED", "forceOn %i", active);
        _forceOn = active;
#ifdef DEBUG_HEARTBEAT_PRIO
        _debugEffect.init(active ? DEBUG_HEARTBEAT_PRIO_ON_FREQ : DEBUG_HEARTBEAT_PRIO_OFF_FREQ);
#endif
    }

    void Led::errorCode(uint8_t code /* = 0 */)
    {
        if (code > 0)
        {
            openknx.logger.log(LogLevel::Trace, "LED", "errorCode %i", code);
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
        openknx.logger.log(LogLevel::Trace, "LED", "on");
        _state = active;
        _effect = LedEffect::Normal;
    }

    void Led::pulsing(uint16_t frequency)
    {
        openknx.logger.log(LogLevel::Trace, "LED", "pulsing (frequency %i)", frequency);
        _state = true;
        _effect = LedEffect::Pulse;
        _pulseEffect.init(frequency);
    }

    void Led::blinking(uint16_t frequency)
    {
        openknx.logger.log(LogLevel::Trace, "LED", "blinking (frequency %i)", frequency);
        _state = true;
        _effect = LedEffect::Blink;
        _blinkEffect.init(frequency);
    }

    void Led::off()
    {
        openknx.logger.log(LogLevel::Trace, "LED", "off");
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

#if defined(DEBUG_HEARTBEAT) || defined(DEBUG_HEARTBEAT_PRIO)
    void Led::debugLoop()
    {
        // Enable Debug Mode on first run
        if (!_debugMode)
        {

#if defined(DEBUG_HEARTBEAT_PRIO)
            _debugEffect.init(DEBUG_HEARTBEAT_PRIO_OFF_FREQ);
#else
            _debugEffect.init(DEBUG_HEARTBEAT_FREQ);
#endif
            _debugMode = true;
        }
        _debugHeartbeat = millis();
    }
#endif

} // namespace OpenKNX