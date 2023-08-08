#pragma once
namespace OpenKNX
{
    class LedHardware
    {
      protected:
#ifdef ARDUINO_ARCH_ESP32
        uint8_t _currentBrightness = 0;
#endif
        long _pin = -1;
        long _activeOn = HIGH;
      public:
        LedHardware(long pin, long activeOn = HIGH);
        virtual void init();
        virtual void write(uint8_t brightness);
        virtual std::string logPrefix();
    };
}