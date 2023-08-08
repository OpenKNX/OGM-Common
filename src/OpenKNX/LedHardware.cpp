#include "OpenKNX/Led.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    LedHardware::LedHardware(long pin, long activeOn)
        : _pin(pin), _activeOn(activeOn)
    {

    }

    void LedHardware::init()
    {
        pinMode(_pin, OUTPUT);
        write(0);
    }
  
    void LedHardware::write(uint8_t brightness)
    {
        #ifdef ARDUINO_ARCH_ESP32
        // Special Hack for ESP32
        // Need to reset pinMode after using analogWrite
        if (_currentBrightness != 0 || _currentBrightness != 255)
            pinMode(_pin, OUTPUT);
        _currentBrightness = brightness;
        #endif

        if (brightness == 255)
            digitalWrite(_pin, _activeOn == HIGH ? true : false);
        else if (brightness == 0)
            digitalWrite(_pin, _activeOn == HIGH ? false : true);
        else
            analogWrite(_pin, _activeOn == HIGH ? brightness : (255 - brightness));
    }

    std::string LedHardware::logPrefix()
    {
        return openknx.logger.buildPrefix("LED", _pin);
    }

} // namespace OpenKNX