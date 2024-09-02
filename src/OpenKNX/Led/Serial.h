#pragma once
#include "OPenKNX/Led/Base.h"
#include <driver/rmt.h>

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
              TimerHandle_t _timer;
              void fillRmt();
            public:
              void init(uint8_t ledPin, uint8_t rmtChannel, uint8_t ledCount);
              void setLED(uint8_t ledAdr, uint8_t r, uint8_t g, uint8_t b);
              void writeLeds(); // send the color data to the LEDs
        };

        class Serial: public Base
        {
            public:  Serial(SerialLedManager* manager) : _manager(manager){};
            private: SerialLedManager* _manager = nullptr;
            private: uint8_t color[3] = {0, 0, 0};  // R, G, B
            private: void writeLed(uint8_t brightness) override;
            public:  void init(long pin = -1, long activeOn = HIGH) override;
            public:  void setColor(uint8_t r, uint8_t g, uint8_t b);
        };
    } // namespace Led
} // namespace OpenKNX