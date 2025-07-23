#pragma once
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
#include "OpenKNX/Led/Base.h"
#if defined(ARDUINO_ARCH_ESP32)
    #include <driver/rmt_tx.h>
#else
    #include "hardware/pio.h"
#endif
#include <stdint.h>

namespace OpenKNX
{
    namespace Led
    {
        class SerialLedManager
        {
          private:
          #if defined(ARDUINO_ARCH_ESP32)
            rmt_symbol_word_t *_rmtItems = nullptr;
            TimerHandle_t _timer;

            rmt_channel_handle_t _led_chan;
            rmt_transmit_config_t _tx_config;
            rmt_encoder_handle_t _simple_encoder;
          #else
            PIO _pio;
            uint _sm;
            uint _offset;
          #endif
            uint8_t _ledCount = 0;
            uint32_t _lastWritten = 0;
            uint8_t* _ledData = 0;
            uint32_t _dirty = 0;


          public:
            void init(uint8_t ledPin, uint8_t ledCount);
            void setLED(uint8_t ledAdr, uint8_t r, uint8_t g, uint8_t b);
            void writeLeds(); // send the color data to the LEDs
        };

        class Serial : public Base
        {
          private:
            SerialLedManager *_manager = nullptr;

          private:
            uint8_t color[3] = {0, 0, 0}; // R, G, B
          private:
            void writeLed(uint8_t brightness) override;

          public:
            void init(long num, SerialLedManager *manager, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);

          public:
            void setColor(uint8_t r, uint8_t g, uint8_t b);
        };
    } // namespace Led
} // namespace OpenKNX
#endif
