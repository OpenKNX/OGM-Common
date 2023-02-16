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

        // Debug (highPrio)
#if defined(DEBUG_HEARTBEAT_PRIO)
        // debug mode enable
        if (_debugMode)
        {
// heartbeat expire -> blink
#if DEBUG_HEARTBEAT_PRIO > 1
            if (!delayCheck(_debugHeartbeat, DEBUG_HEARTBEAT_PRIO))
#else
            if (!delayCheck(_debugHeartbeat, 1000))
#endif
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

        // PowerSave (Prio 1)
        if (_powerSave)
        {
            writeLed(false);
            return;
        }

        // ForceOn (Prio 2)
        if (_forceOn)
        {
            writeLed(true);
            return;
        }

        // FatalError (Prio 3)
        if (_errorCode)
        {
            writeLed(_errorEffect.value());
            return;
        }

        // Normal with optional Effect (Prio 4)
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

        // Debug (lowPrio)
#if defined(DEBUG_HEARTBEAT) && !defined(DEBUG_HEARTBEAT_PRIO)
        // debug mode enable
        if (_debugMode)
        {
// heartbeat expire -> blink
#if DEBUG_HEARTBEAT > 1
            if (delayCheck(_debugHeartbeat, DEBUG_HEARTBEAT))
#else
            if (delayCheck(_debugHeartbeat, 1000))
#endif
            {
                writeLed(_debugEffect.value());
                return;
            }
        }
#endif

        // OFF (Prio 6)
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
            _debugEffect.init(200);
            _debugMode = true;
        }
        _debugHeartbeat = millis();
    }
#endif

} // namespace OpenKNX