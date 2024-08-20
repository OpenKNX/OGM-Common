#include "OpenKNX/Led.h"
#include "OpenKNX/Facade.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

namespace OpenKNX
{
    void __time_critical_func(iLed::loop)()
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

    void iLed::brightness(uint8_t brightness)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("brightness %i", _brightness);
        _brightness = brightness;
    }

    void iLed::powerSave(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("powerSave %i", active);
        _powerSave = active;
    }

    void iLed::forceOn(bool active /* = true */)
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

    void iLed::errorCode(uint8_t code /* = 0 */)
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

    void iLed::on(bool active /* = true */)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("on");
        unloadEffect();
        _state = active;
    }

    void iLed::pulsing(uint16_t frequency)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("pulsing (frequency %i)", frequency);
        loadEffect(new LedEffects::Pulse(frequency));
        _state = true;
    }

    void iLed::blinking(uint16_t frequency)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("blinking (frequency %i)", frequency);
        loadEffect(new LedEffects::Blink(frequency));
        _state = true;
    }

    void iLed::flash(uint16_t duration)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("flash (duration %i ms)", duration);
        loadEffect(new LedEffects::Flash(duration));
        _state = true;
    }

    void iLed::activity(uint32_t &lastActivity, bool inverted)
    {
        // no valid pin
        if (_pin < 0) return;

        logTraceP("activity");
        loadEffect(new LedEffects::Activity(lastActivity, inverted));
        _state = true;
    }

    void iLed::off()
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
    void iLed::writeLed(bool state)
    {
        writeLed((uint8_t)(state ? _brightness : 0));
    }

#ifdef OPENKNX_HEARTBEAT
    void iLed::debugLoop()
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

    void iLed::unloadEffect()
    {
        if (_effectMode)
        {
            logTraceP("unload effect");
            _effectMode = false;
            delete _effect;
        }
    }

    void iLed::loadEffect(LedEffects::Base *effect)
    {
        unloadEffect();
        logTraceP("load effect");
        _effect = effect;
        _effectMode = true;
    }

    std::string iLed::logPrefix()
    {
        return openknx.logger.buildPrefix("LED", _pin);
    }

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

     /*
     * write led state based on bool and _brightness
     */
    void Led::writeLed(uint8_t brightness)
    {
        // no valid pin
        if (_pin < 0) return;

        if (brightness == _currentLedBrightness)
            return;

        // Need to reset pinMode after using analogWrite
        if (_currentLedBrightness != 0 || _currentLedBrightness != 255)
            pinMode(_pin, OUTPUT);

        // logTraceP("==== > %i -> %i\n", _pin, brightness);
        if (brightness == 255)
            digitalWrite(_pin, _activeOn == HIGH ? true : false);

        else if (brightness == 0)
            digitalWrite(_pin, _activeOn == HIGH ? false : true);

        else
            analogWrite(_pin, _activeOn == HIGH ? brightness : (255 - brightness));

        _currentLedBrightness = brightness;
    }




    void RGBLed::init(long pin /* = -1 */, long activeOn /* = HIGH */)
    {
        // no valid pin
        if (pin < 0)
            return;

        _pin = pin;
    }

     /*
     * write led state based on bool and _brightness
     */
    void RGBLed::writeLed(uint8_t brightness)
    {
        // no valid pin
        if (_pin < 0) return;

        if(_currentLedBrightness != brightness)
        {
            _manager->setLED(_pin, (color[0]*(uint16_t)brightness)/256, (color[1]*(uint16_t)brightness)/256, (color[2]*(uint16_t)brightness)/256);

            _currentLedBrightness = brightness;
        }

    }

    #include <driver/rmt.h>
    #define BITS_PER_LED_CMD 24

    // WS2812 timing parameters
    // 0.35us and 0.90us
    // on tick is 80MHz / divider = 0.025us
    #define T0H 14  // 0 bit high time
    #define T0L 36  // 0 bit low time
    #define T1H 36  // 1 bit high time
    #define T1L 14  // 1 bit low time

    /*
    * Set the color of the RGB LED
    */
    void RGBLed::setColor(uint8_t r, uint8_t g, uint8_t b)
    {
        color[0] = r;
        color[1] = g;
        color[2] = b;
        writeLed(_currentLedBrightness);
    }

    void tx_end_callback(rmt_channel_t channel, void* arg)
    {
        rmt_tx_stop(channel);
        digitalWrite(7, 1);
    }

    void RGBLedManager::init(uint8_t ledPin, uint8_t rmtChannel, uint8_t ledCount)
    {
        pinMode(7, OUTPUT);
        digitalWrite(7, 0);
        _rmtChannel = rmtChannel;
        _ledCount = ledCount;
        rmt_config_t config = RMT_DEFAULT_CONFIG_TX((gpio_num_t)ledPin, (rmt_channel_t)rmtChannel);
        config.clk_div = 2; // Set clock divider, affects the timings
        config.mem_block_num = ((ledCount * BITS_PER_LED_CMD) / 64)+1; // one memblock has 64 * 32-bit values (rmt items) which represent 1 encoded bit for ws2812 led. 24bit per LED
        // ToDo calculation

        _rmtItems = new rmt_item32_t[_ledCount * BITS_PER_LED_CMD+1];
        _ledData = new uint32_t[_ledCount];

        //initalize all LEDs off
        for (int i = 0; i < BITS_PER_LED_CMD*_ledCount; i++)
        {
            _rmtItems[i].level0 = 1;
            _rmtItems[i].duration0 = T0H;
            _rmtItems[i].level1 = 0;
            _rmtItems[i].duration1 = T1H;
        }
        //_rmtItems[BITS_PER_LED_CMD*_ledCount].level0 = 0;
        //_rmtItems[BITS_PER_LED_CMD*_ledCount].duration0 = 0xFF;
        //_rmtItems[BITS_PER_LED_CMD*_ledCount].level1 = 0;
        //_rmtItems[BITS_PER_LED_CMD*_ledCount].duration1 = 0xFF;

        // stop item
        //_rmtItems[BITS_PER_LED_CMD*_ledCount].level0 = 0;
        //_rmtItems[BITS_PER_LED_CMD*_ledCount].duration0 = 0;
  
        // Initialize the RMT driver
        rmt_config(&config); // ToDo Error Handling
        rmt_driver_install(config.channel, 0, 0); // ToDo Error Handling
        rmt_register_tx_end_callback(tx_end_callback, NULL);

        writeLeds();

        // Timer-Handle erstellen
        _timer = xTimerCreate(
        "RGBLedManager",         // Name des Timers
        pdMS_TO_TICKS(10),     // Timer-Periode in Millisekunden (hier 1 Sekunde)
        pdTRUE,                  // Auto-Reload (Wiederholung nach Ablauf)
        (void *) 0,              // Timer-ID (kann für Identifikation verwendet werden)
        [](TimerHandle_t timer) {
            openknx.ledManager.writeLeds();
        }            // Callback-Funktion, die beim Timeout aufgerufen wird
    );

    // Überprüfen, ob der Timer erfolgreich erstellt wurde
    if (_timer == NULL) {
        //ESP_LOGE("TIMER", "Fehler beim Erstellen des Timers!");
        return;
    }

    // Timer starten
    if (xTimerStart(_timer, 0) != pdPASS) {
        //ESP_LOGE("TIMER", "Fehler beim Starten des Timers!");
        return;
    }
    }

    void RGBLedManager::setLED(uint8_t ledAdr, uint8_t r, uint8_t g, uint8_t b)
    {
        _ledData[ledAdr] = (g << 16) | (r << 8) | b;
    }

    void RGBLedManager::fillRmt()
    {
        for(int j = 0; j < _ledCount; j++)
        {
            uint32_t colorbits = _ledData[j];
            for (int i = 0; i < BITS_PER_LED_CMD; i++)
            {
                if(colorbits & (1 << (23 - i)))
                {
                    _rmtItems[j*BITS_PER_LED_CMD + i].duration0 = T0L;
                    _rmtItems[j*BITS_PER_LED_CMD + i].duration1 = T1L;
                }
                else
                {
                    _rmtItems[j*BITS_PER_LED_CMD + i].duration0 = T0H;
                    _rmtItems[j*BITS_PER_LED_CMD + i].duration1 = T1H;
                }
                //_rmtItems[ledAdr*BITS_PER_LED_CMD + i].level0 = 1;    //it is always 1, initialized at init
                //_rmtItems[ledAdr*BITS_PER_LED_CMD + i].level1 = 0;    //it is always 0, initialized at init
            }
        }
    }

    void RGBLedManager::writeLeds()
    {
        //if(delayCheckMillis(_lastWritten, 5))
        //{
        //    _lastWritten = millis();
            //uint32_t t1 = micros();
            fillRmt();
            //uint32_t t2 = micros();
            //rmt_write_items((rmt_channel_t)_rmtChannel, _rmtItems, _ledCount * BITS_PER_LED_CMD, false);
            //uint32_t t3 = micros();
        //}
        //Serial.print("fillRmt: ");
        //Serial.print(t2-t1);
        //Serial.print("us. rmt write: ");
        //Serial.print(t3-t2);
        //Serial.println("us");

        //rmt_tx_stop((rmt_channel_t)_rmtChannel);
        rmt_fill_tx_items((rmt_channel_t)_rmtChannel, _rmtItems, _ledCount * BITS_PER_LED_CMD, 0);
        rmt_tx_start((rmt_channel_t)_rmtChannel, true);
        
        //rmt_wait_tx_done((rmt_channel_t)_rmtChannel, portMAX_DELAY);
        //rmt_tx_stop((rmt_channel_t)_rmtChannel);
    }


/*
    void RGBLedManager::writeLeds()
    {
        #ifndef USE_TX_START
        fillRmt();
        rmt_write_items((rmt_channel_t)_rmtChannel, _rmtItems, _ledCount * BITS_PER_LED_CMD, false);
        #else
        //rmt_tx_stop((rmt_channel_t)_rmtChannel);  // maybe stop before? no effect...
        rmt_fill_tx_items((rmt_channel_t)_rmtChannel, _rmtItems, _ledCount * BITS_PER_LED_CMD + 1, 0);
        rmt_tx_start((rmt_channel_t)_rmtChannel, true);

        //rmt_wait_tx_done((rmt_channel_t)_rmtChannel, portMAX_DELAY); // wait and stop, just for testing, no effect...
        //rmt_tx_stop((rmt_channel_t)_rmtChannel);
        #endif
    }
    */


} // namespace OpenKNX