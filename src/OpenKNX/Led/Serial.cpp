#ifdef ARDUINO_ARCH_ESP32

#include "OpenKNX/Led/Serial.h"
#include "OpenKNX/Facade.h"

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define BITS_PER_LED_CMD 24

namespace OpenKNX
{
    namespace Led
    {
        static const rmt_symbol_word_t ws2812_zero = {
            .duration0 = (uint16_t)3, // T0H=0.3us
            .level0 = 1,
            .duration1 = (uint16_t)9, // T0L=0.9us
            .level1 = 0,
        };

        static const rmt_symbol_word_t ws2812_one = {
            .duration0 = (uint16_t)9, // T1L=0.9us
            .level0 = 1,
            .duration1 = (uint16_t)3, // T1H=0.3us
            .level1 = 0,
        };

        //reset defaults to 50uS
        static const rmt_symbol_word_t ws2812_reset = {
            .duration0 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
            .level0 = 1,
            .duration1 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
            .level1 = 0,
        };

        static size_t encoder_callback(const void *data, size_t data_size,
                               size_t symbols_written, size_t symbols_free,
                               rmt_symbol_word_t *symbols, bool *done, void *arg)
        {
            // We need a minimum of 8 symbol spaces to encode a byte. We only
            // need one to encode a reset, but it's simpler to simply demand that
            // there are 8 symbol spaces free to write anything.
            if (symbols_free < 8)
            {
                return 0;
            }

            // We can calculate where in the data we are from the symbol pos.
            // Alternatively, we could use some counter referenced by the arg
            // parameter to keep track of this.
            size_t data_pos = symbols_written / 8;
            uint8_t *data_bytes = (uint8_t*)data;
            if (data_pos < data_size)
            {
                // Encode a byte
                size_t symbol_pos = 0;
                for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1)
                {
                    if (data_bytes[data_pos]&bitmask)
                    {
                        symbols[symbol_pos++] = ws2812_one;
                    } else {
                        symbols[symbol_pos++] = ws2812_zero;
                    }
                }
                // We're done; we should have written 8 symbols.
                return symbol_pos;
            }
            else
            {
                //All bytes already are encoded.
                //Encode the reset, and we're done.
                symbols[0] = ws2812_reset;
                *done = 1; //Indicate end of the transaction.
                return 1; //we only wrote one symbol
            }
        }

        void Serial::init(long num, SerialLedManager *manager, uint8_t r, uint8_t g, uint8_t b)
        {
            // no valid pin
            if (num < 0 || manager == nullptr)
                return;

            _pin = num;
            _manager = manager;
            setColor(r, g, b);
        }

        /*
         * write led state based on bool and _brightness
         */
        void Serial::writeLed(uint8_t brightness)
        {
            // no valid pin
            if (_pin < 0 || _manager == nullptr) return;

            if (_currentLedBrightness != brightness)
            {
                _manager->setLED(
                    _pin,
                    ((uint32_t)color[0] * brightness * _maxBrightness / 100 / 256),
                    ((uint32_t)color[1] * brightness * _maxBrightness / 100 / 256),
                    ((uint32_t)color[2] * brightness * _maxBrightness / 100 / 256));

                _currentLedBrightness = brightness;
            }
        }



        /*
         * Set the color of the RGB LED
         */
        void Serial::setColor(uint8_t r, uint8_t g, uint8_t b)
        {
            color[0] = r;
            color[1] = g;
            color[2] = b;
            _manager->setLED(_pin, (color[0] * (uint16_t)_currentLedBrightness) / 256, (color[1] * (uint16_t)_currentLedBrightness) / 256, (color[2] * (uint16_t)_currentLedBrightness) / 256);
        }

        void SerialLedManager::init(uint8_t ledPin, uint8_t ledCount)
        {
            logError("SerialLedManager", "init");
            _ledCount = ledCount;
            _led_chan = NULL;
            _ledData = new uint8_t[_ledCount*3];
            rmt_tx_channel_config_t tx_chan_config =
                    {
                        .gpio_num = (gpio_num_t)ledPin,
                        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
                        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
                        .mem_block_symbols = (size_t)(ledCount * BITS_PER_LED_CMD) , // increase the block size can make the LED less flickering
                        .trans_queue_depth = 1, // set the number of transactions that can be pending in the background
                    };
            rmt_new_tx_channel(&tx_chan_config, &_led_chan);
            _simple_encoder = NULL;
            const rmt_simple_encoder_config_t simple_encoder_cfg =
            {
                .callback = encoder_callback
                //min_chunk_size default 64 is good
            };
            rmt_new_simple_encoder(&simple_encoder_cfg, &_simple_encoder);

            rmt_enable(_led_chan);

            _tx_config =
            {
                .loop_count = 0, // no transfer loop
            };            

            // Timer-Handle erstellen
            _timer = xTimerCreate(
                "SerialLedManager", // Name des Timers
                pdMS_TO_TICKS(10),  // Timer-Periode in Millisekunden 
                pdTRUE,             // Auto-Reload (Wiederholung nach Ablauf)
                (void *)0,          // Timer-ID (kann für Identifikation verwendet werden)
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
    #ifdef OPENKNX_SERIALLED_ENABLE
                    openknx.ledManager.writeLeds();
    #endif
                } // Callback-Funktion, die beim Timeout aufgerufen wird
            );

            // Überprüfen, ob der Timer erfolgreich erstellt wurde
            if (_timer == NULL)
            {
                logError("SerialLedManager", "Timer creation failed");
                return;
            }

            // Timer starten
            if (xTimerStart(_timer, 0) != pdPASS)
            {
                logError("SerialLedManager", "Could not start Timer");
                return;
            }
            
        }

        void SerialLedManager::setLED(uint8_t ledAdr, uint8_t r, uint8_t g, uint8_t b)
        {
           _dirty = false;
           if(_ledData[ledAdr*3] != g)
            {
                _ledData[ledAdr*3] = g;
                _ledData[ledAdr*3+1] = r;
                _ledData[ledAdr*3+2] = b;
                _dirty = true;
                return;
            }
            if(_ledData[ledAdr*3+1] != r)
            {
                _ledData[ledAdr*3+1] = r;
                _ledData[ledAdr*3+2] = b;
                _dirty = true;
                return;
            }
            if(_ledData[ledAdr*3+2] != b)
            {
                _ledData[ledAdr*3+2] = b;
                _dirty = true;
            }
        }

        void SerialLedManager::writeLeds()
        {
            if (!_dirty)
                return;

            if (delayCheckMillis(_lastWritten, 5)) // prevent calling a new rmt transmission into an running on
            {
                _lastWritten = millis();
                _dirty = 0;

                // Flush RGB values to LEDs
                //uint32_t t1 = micros();
                rmt_transmit(_led_chan, _simple_encoder, _ledData, _ledCount*3, &_tx_config);
                //uint32_t t2 = micros();

                //::Serial.print("rmt_transmit: "); ~50us
                //::Serial.print(t2-t1);
            }
        }
    } // namespace Led
} // namespace OpenKNX

#endif