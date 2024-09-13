#include "OpenKNX/Led/Base.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    namespace Led
    {
        void __time_critical_func(Base::loop)()
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

        void Base::brightness(uint8_t brightness)
        {
            // no valid pin
            if (_pin < 0) return;

            logTraceP("brightness %i", _brightness);
            _brightness = brightness;
        }

        void Base::powerSave(bool active /* = true */)
        {
            // no valid pin
            if (_pin < 0) return;

            logTraceP("powerSave %i", active);
            _powerSave = active;
        }

        void Base::forceOn(bool active /* = true */)
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

        void Base::errorCode(uint8_t code /* = 0 */)
        {
            // no valid pin
            if (_pin < 0) return;

            _errorMode = false;
            if (_errorMode) delete _errorEffect;

            if (code > 0)
            {
                logTraceP("errorCode %i", code);
                _errorEffect = new Led::Effects::Error(code);
                _errorMode = true;
            }
        }

        void Base::on(bool active /* = true */)
        {
            // no valid pin
            if (_pin < 0) return;

            logTraceP("on");
            unloadEffect();
            _state = active;
        }

        void Base::pulsing(uint16_t frequency)
        {
            // no valid pin
            if (_pin < 0) return;

            logTraceP("pulsing (frequency %i)", frequency);
            loadEffect(new Led::Effects::Pulse(frequency));
            _state = true;
        }

        void Base::blinking(uint16_t frequency)
        {
            // no valid pin
            if (_pin < 0) return;

            logTraceP("blinking (frequency %i)", frequency);
            loadEffect(new Led::Effects::Blink(frequency));
            _state = true;
        }

        void Base::flash(uint16_t duration)
        {
            // no valid pin
            if (_pin < 0) return;

            logTraceP("flash (duration %i ms)", duration);
            loadEffect(new Led::Effects::Flash(duration));
            _state = true;
        }

        void Base::activity(uint32_t &lastActivity, bool inverted)
        {
            // no valid pin
            if (_pin < 0) return;

            logTraceP("activity");
            loadEffect(new Led::Effects::Activity(lastActivity, inverted));
            _state = true;
        }

        void Base::off()
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
        void Base::writeLed(bool state)
        {
            writeLed((uint8_t)(state ? _brightness : 0));
        }

#ifdef OPENKNX_HEARTBEAT
        void Base::debugLoop()
        {
            // Enable Debug Mode on first run
            if (!_debugMode)
            {
    #ifdef OPENKNX_HEARTBEAT_PRIO
                _debugEffect = new Led::Effects::Blink(OPENKNX_HEARTBEAT_PRIO_OFF_FREQ);
    #else
                _debugEffect = new Led::Effects::Blink(OPENKNX_HEARTBEAT_FREQ);
    #endif
                _debugMode = true;
            }

            _debugHeartbeat = millis();
        }
#endif

        void Base::unloadEffect()
        {
            if (_effectMode)
            {
                logTraceP("unload effect");
                _effectMode = false;
                delete _effect;
            }
        }

        void Base::loadEffect(Led::Effects::Base *effect)
        {
            unloadEffect();
            logTraceP("load effect");
            _effect = effect;
            _effectMode = true;
        }

        std::string Base::logPrefix()
        {
            return openknx.logger.buildPrefix("LED", _pin);
        }
    } // namespace Led
} // namespace OpenKNX