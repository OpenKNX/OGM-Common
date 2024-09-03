#pragma once
#include "OpenKNX/Led/Base.h"
#ifdef ARDUINO_ARCH_ESP32
    #include <driver/rmt.h>
#else
    #error "OpenKNX::LED::Serial only supported for ESP32 architecture"
#endif

namespace OpenKNX
{
    namespace Led
    {

        class SerialLedManager
        {
          private:
            rmt_item32_t *_rmtItems = nullptr;
            uint8_t _rmtChannel = 0;
            uint8_t _ledCount = 0;
            uint32_t _lastWritten = 0;
            uint32_t *_ledData = nullptr;
            uint32_t _dirty = 0;
            TimerHandle_t _timer;
            void fillRmt();

          public:
            void init(uint8_t ledPin, uint8_t rmtChannel, uint8_t ledCount);
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