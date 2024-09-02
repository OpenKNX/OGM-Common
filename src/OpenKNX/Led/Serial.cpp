#include "OpenKNX/Led/Serial.h"
#include "OpenKNX/Facade.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

namespace OpenKNX
{
    namespace Led
    {
        void Serial::init(long num, SerialLedManager* manager, uint8_t r, uint8_t g, uint8_t b)
        {
            // no valid pin
            if (num < 0 || manager == nullptr)
                return;

            _pin = num;
            _manager = manager;
            setColor(r,g,b);
        }

        /*
        * write led state based on bool and _brightness
        */
        void Serial::writeLed(uint8_t brightness)
        {
            // no valid pin
            if (_pin < 0 || _manager == nullptr)
                return;

            if(_currentLedBrightness != brightness)
            {
                _manager->setLED(_pin, (color[0]*(uint16_t)brightness)/256, (color[1]*(uint16_t)brightness)/256, (color[2]*(uint16_t)brightness)/256);

                _currentLedBrightness = brightness;
            }

        }

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
        void Serial::setColor(uint8_t r, uint8_t g, uint8_t b)
        {
            color[0] = r;
            color[1] = g;
            color[2] = b;
            _manager->setLED(_pin, (color[0]*(uint16_t)_currentLedBrightness)/256, (color[1]*(uint16_t)_currentLedBrightness)/256, (color[2]*(uint16_t)_currentLedBrightness)/256);
        }

        void SerialLedManager::init(uint8_t ledPin, uint8_t rmtChannel, uint8_t ledCount)
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

            writeLeds();

            // Timer-Handle erstellen
            _timer = xTimerCreate(
            "SerialLedManager",         // Name des Timers
            pdMS_TO_TICKS(10),     // Timer-Periode in Millisekunden (hier 1 Sekunde)
            pdTRUE,                  // Auto-Reload (Wiederholung nach Ablauf)
            (void *) 0,              // Timer-ID (kann für Identifikation verwendet werden)
            [](TimerHandle_t timer) {
                openknx.progLed.loop();
    #ifdef INFO2_LED_PIN
                openknx.info2Led.loop();
    #endif
    #ifdef INFO1_LED_PIN
                openknx.info1Led.loop();
    #endif
    #ifdef INFO3_LED_PIN
                openknx.info3Led.loop();
    #endif
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

        void SerialLedManager::setLED(uint8_t ledAdr, uint8_t r, uint8_t g, uint8_t b)
        {
            _ledData[ledAdr] = (g << 16) | (r << 8) | b;
        }

        void SerialLedManager::fillRmt()
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
                }
            }
        }

        void SerialLedManager::writeLeds()
        {
            if(delayCheckMillis(_lastWritten, 5))
            {
                _lastWritten = millis();
                //uint32_t t1 = micros();
                fillRmt();
                //uint32_t t2 = micros();
                rmt_write_items((rmt_channel_t)_rmtChannel, _rmtItems, _ledCount * BITS_PER_LED_CMD, false);
                //uint32_t t3 = micros();
            }
            //Serial.print("fillRmt: ");
            //Serial.print(t2-t1);
            //Serial.print("us. rmt write: ");
            //Serial.print(t3-t2);
            //Serial.println("us");
        }
    } // namespace Led
} // namespace OpenKNX