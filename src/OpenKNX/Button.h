#pragma once
#include "OpenKNX/defines.h"
#include <Arduino.h>
#include <functional>
#include <string>

#define OPENKNX_BUTTON_DEBOUNCE 50
#define OPENKNX_BUTTON_DOUBLE_CLICK 500
#define OPENKNX_BUTTON_LONG_CLICK 1000

typedef std::function<void(void)> ShortClickCallbackFunction;
typedef std::function<void(void)> LongClickCallbackFunction;
typedef std::function<void(void)> DoubleClickCallbackFunction;

namespace OpenKNX
{
    class Button
    {
      private:
        const char *_id;
        bool _hardwareStatus = false;
        bool _currentStatus = false;
        uint32_t _lastDebounceTime = 0;

        bool _pressed = false;
        bool _processed = false;
        volatile uint32_t _debounceTimer = 0;
        volatile uint32_t _holdTimer = 0;
        volatile uint32_t _longTimer = 0;
        volatile uint32_t _dblClickTimer = 0;

        ShortClickCallbackFunction _shortClickCallback = nullptr;
        LongClickCallbackFunction _longClickCallback = nullptr;
        DoubleClickCallbackFunction _doubleClickCallback = nullptr;

        inline void callShortClickCallback();
        inline void callLongClickCallback();
        inline void callDoubleClickCallback();

      public:
        Button(const char *id) : _id(id){};
        void change(bool pressed);
        void loop();

        void onShortClick(ShortClickCallbackFunction shortClickCallback) { _shortClickCallback = shortClickCallback; }
        void onLongClick(LongClickCallbackFunction longClickCallback) { _longClickCallback = longClickCallback; }
        void onDoubleClick(DoubleClickCallbackFunction doubleCallback) { _doubleClickCallback = doubleCallback; }

        std::string logPrefix();
    };
} // namespace OpenKNX