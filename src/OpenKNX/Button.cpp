#include "OpenKNX/Button.h"
#include "OpenKNX/Facade.h"

namespace OpenKNX
{
    void __time_critical_func(Button::loop)()
    {
        if (_pressed)
        {
            // first press?
            if (!_holdTimer)
            {
                _processed = false;
                _holdTimer = millis();
                // #ifdef OPENKNX_RTT
                logTraceP("  Pressed");
                // #endif
            }
        }
        else
        {
            if (_holdTimer)
            {
                if (_doubleClickCallback == nullptr)
                {
                    // No wait for double click for faster response
                    if (!_processed && _holdTimer && delayCheck(_holdTimer, OPENKNX_BUTTON_DEBOUNCE))
                    {
                        callShortClickCallback();
                    }
                }
                else
                {
                    if (_dblClickTimer)
                    {
                        _dblClickTimer = 0;
                        callDoubleClickCallback();
                        _processed = true;
                    }
                    else
                    {
                        _dblClickTimer = millis();
                    }
                }
            }

            _holdTimer = 0;
        }

        if (!_processed)
        {
            if (_dblClickTimer && delayCheck(_dblClickTimer, OPENKNX_BUTTON_DOUBLE_CLICK))
            {
                callShortClickCallback();
                _dblClickTimer = 0;
                _processed = true;
            }

            if (_holdTimer && delayCheck(_holdTimer, OPENKNX_BUTTON_LONG_CLICK))
            {
                callLongClickCallback();
                _processed = true;
            }
        }
    }

    void __time_critical_func(Button::change)(bool status)
    {
        _pressed = status;
    }

    void Button::callShortClickCallback()
    {
        // #ifdef OPENKNX_RTT
        logTraceP("ShortClick");
        // #endif
        if (_shortClickCallback != nullptr) _shortClickCallback();
    }

    void Button::callLongClickCallback()
    {
        // #ifdef OPENKNX_RTT
        logTraceP("LongClick");
        // #endif
        if (_longClickCallback != nullptr) _longClickCallback();
    }

    void Button::callDoubleClickCallback()
    {
        // #ifdef OPENKNX_RTT
        logTraceP("DoubleClick");
        // #endif
        if (_doubleClickCallback != nullptr) _doubleClickCallback();
    }

    std::string Button::logPrefix()
    {
        return openknx.logger.buildPrefix("Button", _id);
    }

} // namespace OpenKNX