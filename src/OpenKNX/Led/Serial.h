#pragma once
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
    #include "OpenKNX/Led/RGB.h"
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
            long _ledPin = -1;
            uint8_t _ledCount = 0;
            uint32_t _lastWritten = 0;
            uint8_t *_ledData = 0;
            uint32_t _dirty = 0;

          public:
            SerialLedManager(long ledPin) : _ledPin(ledPin) {}
            void init(uint8_t ledCount);
            void setLED(uint8_t ledAddr, uint8_t r, uint8_t g, uint8_t b);
            void writeLeds(); // send the color data to the LEDs
            long getPin() { return _ledPin; }
        };

        class Serial : public RGB
        {
          protected:
            SerialLedManager *_manager = nullptr;
            uint8_t _color[3] = {0, 0, 0}; // R, G, B
            long _addr = -1;
            long _pin = -1;

          protected:
            void writeLed(uint8_t brightness) override;

          public:
            Serial(long num, long pin, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
            Serial(long num, long pin, Color rgb);

          public:
            void setManager(SerialLedManager *manager);

          public:
            void init();

          public:
            long getAddr() { return _addr; }
            long getPin() { return _pin; }

          public:
            void color(uint8_t r, uint8_t g, uint8_t b) override;
        };
    } // namespace Led
} // namespace OpenKNX
#endif
