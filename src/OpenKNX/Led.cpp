#include "OpenKNX/Led.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    void Led::init(long pin /* = -1 */, long activeOn /* = HIGH */)
    {
        // no valid pin
        if (pin < 0)
        {
            init((LedHardware*) NULL);
            return;
        }
        init(new LedHardware(pin, activeOn));
    }

    void Led::init(LedHardware *ledHardware)
    {
        if (_ledHardware != NULL)
            delete _ledHardware;
        _ledHardware = ledHardware;
        if (_ledHardware != NULL)
            _ledHardware->init();
    }
    
    Led::~Led()
    {
        if (_ledHardware != NULL)
        {
            delete _ledHardware;
            _ledHardware = NULL;
        }
    }

#ifdef __time_critical_func
    void __time_critical_func(Led::loop)()
#else
    void Led::loop()
#endif
    {
        // IMPORTANT!!! The method millis() and micros() are not incremented further in the interrupt!
        // no valid hardware
        if (_ledHardware == NULL)
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
#ifdef OPENKNX_HEARTBEAT
        // debug mode enable
        if (_debugMode)
        {
    #ifdef OPENKNX_HEARTBEAT_PRIO
            // Blinking until the heartbeat signal stops.
            if (!(millis() - _debugHeartbeat >= OPENKNX_HEARTBEAT))
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
            if ((millis() - _debugHeartbeat >= OPENKNX_HEARTBEAT))
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
        // no valid hardware
        if (_ledHardware == NULL)
            return;

        logTraceP("brightness %i", _brightness);
        _brightness = brightness;
    }

    void Led::powerSave(bool active /* = true */)
    {
        // no valid hardware
        if (_ledHardware == NULL)
            return;

        logTraceP("powerSave %i", active);
        _powerSave = active;
    }

    void Led::forceOn(bool active /* = true */)
    {
        // no valid hardware
        if (_ledHardware == NULL)
            return;

        logTraceP("forceOn %i", active);
        _forceOn = active;
#ifdef OPENKNX_HEARTBEAT_PRIO
        _debugEffect.init(active ? OPENKNX_HEARTBEAT_PRIO_ON_FREQ : OPENKNX_HEARTBEAT_PRIO_OFF_FREQ);
#endif
    }

    void Led::errorCode(uint8_t code /* = 0 */)
    {
        // no valid hardware
        if (_ledHardware == NULL)
            return;

        if (code > 0)
        {
            logTraceP("errorCode %i", code);
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
        // no valid hardware
        if (_ledHardware == NULL)
            return;
        logTraceP("on");
        _state = active;
        _effect = LedEffect::Normal;
    }

    void Led::pulsing(uint16_t frequency)
    {
        // no valid hardware
        if (_ledHardware == NULL)
            return;

        logTraceP("pulsing (frequency %i)", frequency);
        _state = true;
        _effect = LedEffect::Pulse;
        _pulseEffect.init(frequency);
    }

    void Led::blinking(uint16_t frequency)
    {
        // no valid hardware
        if (_ledHardware == NULL)
            return;

        logTraceP("blinking (frequency %i)", frequency);
        _state = true;
        _effect = LedEffect::Blink;
        _blinkEffect.init(frequency);
    }

    void Led::off()
    {
        // no valid pin
        if (_ledHardware == NULL)
            return;

        logTraceP("off");
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
        // no valid hardware
        LedHardware* ledHardware = _ledHardware;
        if (ledHardware == NULL)
            return;

        if (brightness == _currentLedBrightness)
            return;

        // logTraceP("==== > %i -> %i\n", _pin, brightness);
        // SERIAL_DEBUG.printf("==== > %i -> %i\n", _pin, brightness);
        ledHardware->write(brightness);
        _currentLedBrightness = brightness;
    }


#ifdef OPENKNX_HEARTBEAT
    void Led::debugLoop()
    {
        // Enable Debug Mode on first run
        if (!_debugMode)
        {

    #ifdef OPENKNX_HEARTBEAT_PRIO
            _debugEffect.init(OPENKNX_HEARTBEAT_PRIO_OFF_FREQ);
    #else
            _debugEffect.init(OPENKNX_HEARTBEAT_FREQ);
    #endif
            _debugMode = true;
        }
        _debugHeartbeat = millis();
    }
#endif

    std::string Led::logPrefix()
    {
        LedHardware* ledHardware = _ledHardware;
        if (ledHardware != NULL)   
            return ledHardware->logPrefix();
        else
            return openknx.logger.buildPrefix("LED", "X");
    }

} // namespace OpenKNX