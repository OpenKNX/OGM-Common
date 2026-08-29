/**
 * @file        WidgetSysInfo.cpp
 * @brief       Multi-page system-information widget provided by OGM-Common
 * @version     0.0.1
 * @date        2026-07-11
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#ifdef DEVICE_DISPLAY_MODULE
    #include "WidgetSysInfo.h"
    #include "OpenKNX.h"

namespace OpenKNX
{

    WidgetSysInfo::WidgetSysInfo(uint32_t displayTime, WidgetFlags action)
        : _state(WidgetState::STOPPED), _displayTime(displayTime), _action(action), _display(nullptr)
    {
    }

    // ---- KNX type label from MASK_VERSION -------------------------------
    std::string WidgetSysInfo::knxTypeLabel(uint16_t maskVersion)
    {
        switch (maskVersion)
        {
            case 0x091A:
                return "Router 091A";
            case 0x07B0:
                return "TP Device 07B0";
            case 0x57B0:
                return "Coupler 57B0";
            default:
            {
                char buffer[8] = {};
                snprintf(buffer, sizeof(buffer), "%04X", maskVersion);
                return std::string(buffer);
            }
        }
    }

    // ---- Widget getters/setters -------------------------------------------------
    uint32_t WidgetSysInfo::getDisplayTime() const
    {
        return _displayTime;
    }

    WidgetFlags WidgetSysInfo::getAction() const
    {
        return _action;
    }

    void WidgetSysInfo::setDisplayModule(i2cDisplay *displayModule)
    {
        _display = displayModule;
        if (_display == nullptr)
        {
            logErrorP("Display is NULL.");
        }
    }

    i2cDisplay *WidgetSysInfo::getDisplayModule() const
    {
        return _display;
    }

    // ---- Multi-page interface -------------------------------------------
    void WidgetSysInfo::setPage(size_t page)
    {
        if (page >= PAGE_COUNT) page = PAGE_COUNT - 1;
        _currentPage = page;
        _lastPageFlip = millis();
    }

    void WidgetSysInfo::nextPage()
    {
        _currentPage = (_currentPage + 1) % PAGE_COUNT;
        _lastPageFlip = millis();
    }

    void WidgetSysInfo::prevPage()
    {
        _currentPage = (_currentPage + PAGE_COUNT - 1) % PAGE_COUNT;
        _lastPageFlip = millis();
    }

    uint32_t WidgetSysInfo::pageDwellMs() const
    {
        // Auto-paging timing: whole widget window split across all pages.
        uint32_t dwell = (_displayTime > 0) ? (_displayTime / PAGE_COUNT) : 1000;
        if (dwell < 500) dwell = 500; // keep each page readable
        return dwell;
    }

    // ---- Lifecycle --------------------------------------------------------------
    void WidgetSysInfo::setup()
    {
        logDebugP("Setup...");
        if (_display == nullptr)
        {
            logErrorP("Display is NULL.");
            return;
        }
    }

    void WidgetSysInfo::start()
    {
        if (_state == WidgetState::RUNNING) return;
        logDebugP("Start...");
        _state = WidgetState::RUNNING;
        _currentPage = 0;
        _startTime = millis();
        _lastPageFlip = _startTime;
        _lastUpdate = 0; // force an immediate redraw on the first loop
    }

    void WidgetSysInfo::stop()
    {
        logDebugP("Stop...");
        _state = WidgetState::STOPPED;
        if (_display && _display->display)
        {
            _display->display->clearDisplay();
            _display->displayBuff();
        }
    }

    void WidgetSysInfo::pause()
    {
        if (_state == WidgetState::RUNNING)
        {
            logDebugP("Pause...");
            _state = WidgetState::PAUSED;
        }
    }

    void WidgetSysInfo::resume()
    {
        if (_state == WidgetState::PAUSED)
        {
            logDebugP("Resume...");
            _state = WidgetState::RUNNING;
        }
    }

    void WidgetSysInfo::loop()
    {
        if (_state != WidgetState::RUNNING || !_display) return;

        uint32_t now = millis();

        // Auto-paging: advance the page once its dwell time elapsed. Pause freezes EVERYTHING
        // — including this internal page advance (the manager's updateAutoPaging is already
        // pause-gated, so the internal timer must be too; "Pause ist Pause").
        bool paused = false;
        if (openknxDisplayModule.getWidgetManager())
            paused = openknxDisplayModule.getWidgetManager()->isRotationPaused();

        if (!paused && (uint32_t)(now - _lastPageFlip) >= pageDwellMs())
        {
            _currentPage = (_currentPage + 1) % PAGE_COUNT;
            _lastPageFlip = now;
            _lastUpdate = 0; // redraw immediately on page change
        }
        else if (paused)
        {
            _lastPageFlip = now; // don't accumulate while paused -> no page jump on resume
        }

        // Redraw 1x/s to refresh live values + countdown.
        if (_lastUpdate == 0 || (uint32_t)(now - _lastUpdate) >= 1000)
        {
            _lastUpdate = now;
            drawSysInfo();
        }
    }

    // ---- Drawing ----------------------------------------------------------------
    void WidgetSysInfo::drawSysInfo()
    {
        if (!_display || !_display->display)
        {
            logErrorP("Display or display driver is NULL.");
            return;
        }

        _display->display->clearDisplay();
        _display->display->setTextColor(WHITE);
        _display->display->setTextSize(1);
        _display->display->setTextWrap(false);

        drawHeader();
        drawPageBody();

        _display->displayBuff();
    }

    void WidgetSysInfo::drawHeader()
    {
        const uint16_t screenW = _display->GetDisplayWidth();

        // Title left, like every other widget.
        const std::string &title = getName();
        _display->display->setCursor(0, 0);
        _display->display->print(title.c_str());

        // Countdown marker travelling along the header divider, mirroring
        // WidgetSDCard::drawSDInfo so all rotating widgets look consistent.
        if (_displayTime > 0)
        {
            uint32_t elapsed = millis() - _startTime;
            if (elapsed > _displayTime) elapsed = _displayTime;
            uint16_t markerX = (uint16_t)(((uint32_t)screenW * elapsed) / _displayTime);
            _display->display->fillCircle(markerX, 10, 2, WHITE);
            _display->display->drawCircle(markerX, 10, 2, BLACK);
        }

        _display->display->drawLine(0, 10, screenW, 10, WHITE);

        // Page indicator: dots, not "n/N". They end at screenW-12 because the manager owns the
        // corner there (pause/play glyph, busmon badge).
        const int16_t startX = (int16_t)screenW - 12 - (int16_t)PAGE_COUNT * 6;
        for (uint8_t i = 0; i < PAGE_COUNT; i++)
        {
            const int16_t x = startX + i * 6 + 2;
            if (i == _currentPage)
                _display->display->fillCircle(x, 4, 2, WHITE);
            else
                _display->display->drawCircle(x, 4, 1, WHITE);
        }
    }

    void WidgetSysInfo::drawKeyValueRow(uint8_t row, const char *key, const std::string &value)
    {
        // Four key/value rows starting below the header divider (y = 10).
        const int16_t y = (int16_t)(15 + row * 12);
        _display->display->setCursor(0, y);
        _display->display->print(key);
        _display->display->print(": ");
        _display->display->print(value.c_str());
    }

    void WidgetSysInfo::drawPageBody()
    {
        switch (_currentPage)
        {
            case 0: // Gerät
            {
                drawKeyValueRow(0, "ID", openknx.info.firmwareName());
                drawKeyValueRow(1, "SN", openknx.info.humanSerialNumber());
                drawKeyValueRow(2, "PA", openknx.info.humanIndividualAddress());
                break;
            }
            case 1: // Firmware
            {
                drawKeyValueRow(0, "Version", openknx.info.humanFirmwareVersion());
                drawKeyValueRow(1, "Nummer", openknx.info.humanFirmwareNumber());
                drawKeyValueRow(2, "KNX", knxTypeLabel(MASK_VERSION));
                break;
            }
            case 2: // Runtime
            {
                char freeBuf[24] = {};
                snprintf(freeBuf, sizeof(freeBuf), "%.1f KiB", (float)freeMemory() / 1024.0f);
                drawKeyValueRow(0, "Free", std::string(freeBuf));

                // PSRAM only exists on ESP32 boards that have it (RP2040 has none) -> the row is
                // omitted entirely there instead of showing a placeholder; Uptime shifts up.
                uint8_t rtRow = 1;
    #if defined(ARDUINO_ARCH_ESP32) && defined(BOARD_HAS_PSRAM)
                char psramBuf[24] = {};
                snprintf(psramBuf, sizeof(psramBuf), "%.0f KiB", (float)ESP.getFreePsram() / 1024.0f);
                drawKeyValueRow(rtRow++, "PSRAM", std::string(psramBuf));
    #endif
                drawKeyValueRow(rtRow, "Uptime", openknx.logger.buildUptime());
                break;
            }
            case 3: // Diagnose -- values the console shows but the display never did
            {
                char buf[24] = {};

                // The MINIMUM matters, not the current value: a leak shows in the low-water mark.
                snprintf(buf, sizeof(buf), "%.1f KiB", (float)openknx.common.freeMemoryMin() / 1024.0f);
                drawKeyValueRow(0, "Heap min", std::string(buf));

    #ifdef OPENKNX_DUALCORE
                snprintf(buf, sizeof(buf), "%i / %i B", openknx.common.freeStackMin(),
                         openknx.common.freeStackMin1());
    #else
                snprintf(buf, sizeof(buf), "%i B", openknx.common.freeStackMin());
    #endif
                drawKeyValueRow(1, "Stack", std::string(buf));

                // Silent restarts are invisible otherwise.
                if (openknx.watchdog.active())
                    snprintf(buf, sizeof(buf), "%u (%us)", (unsigned)openknx.watchdog.resets(),
                             (unsigned)openknx.watchdog.maxPeriod());
                else
                    snprintf(buf, sizeof(buf), "off");
                drawKeyValueRow(2, "Watchdog", std::string(buf));

    #if defined(ARDUINO_ARCH_ESP32)
                snprintf(buf, sizeof(buf), "%s %u MHz", ESP.getChipModel(), (unsigned)getCpuFrequencyMhz());
    #elif defined(ARDUINO_ARCH_RP2040)
                snprintf(buf, sizeof(buf), "RP %u MHz", (unsigned)(F_CPU / 1000000UL));
    #else
                snprintf(buf, sizeof(buf), "-");
    #endif
                drawKeyValueRow(3, "Chip", std::string(buf));
                break;
            }

            default:
                break;
        }
    }

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE
