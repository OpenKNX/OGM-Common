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
    #include "DisplayUi.h"
    #include "Icons/UiIcons.h"
    #include "OpenKNX.h"
    #include <string.h>

namespace OpenKNX
{
    namespace
    {
        /** @brief Card symbol: chip with four pins on each side, in x47..80 y0..27. */
        void drawChipSymbol(Adafruit_SSD1306 &d)
        {
            d.drawRoundRect(52, 4, 24, 20, 2, WHITE);
            d.drawRect(58, 10, 12, 8, WHITE);
            for (int16_t i = 0; i < 4; i++)
            {
                d.drawFastHLine(47, (int16_t)(7 + i * 4), 5, WHITE);
                d.drawFastHLine(76, (int16_t)(7 + i * 4), 5, WHITE);
            }
            for (int16_t i = 0; i < 4; i++)
            {
                d.drawFastVLine((int16_t)(56 + i * 5), 0, 4, WHITE);
                d.drawFastVLine((int16_t)(56 + i * 5), 24, 4, WHITE);
            }
        }

        /**
         * @brief Ui::row() that wraps a value too long for its row onto the next row, at the same column.
         * Breaks at the last space that fits if the rest then fits too, else after a full row; the second row is cut
         * with "...". Returns the rows used.
         */
        uint8_t rowWrap(Adafruit_SSD1306 &d, uint8_t row, const char *label, const char *value, uint8_t labelCols)
        {
            const int16_t room = (int16_t)((d.width() - Ui::ValueEnd) / Ui::CharW - labelCols);
            const int16_t len = (int16_t)strlen(value);
            if (len <= room || room < 4)
            {
                Ui::row(d, row, label, value, labelCols);
                return 1;
            }
            int16_t cut = room, next = room;
            for (int16_t i = room; i > 0; i--)
            {
                if (value[i] == ' ')
                {
                    if (len - i - 1 <= room)
                    {
                        cut = i;
                        next = (int16_t)(i + 1);
                    }
                    break;
                }
            }
            if (value[next] == ' ') next++; // a break inside the word must not start the second row with a space
            Ui::row(d, row, label, nullptr);
            const int16_t x = (int16_t)(labelCols * Ui::CharW);
            const int16_t y = (int16_t)(Ui::Row0 + Ui::Pitch * row);
            d.setCursor(x, y);
            for (int16_t i = 0; i < cut; i++)
                d.write(value[i]);
            Ui::printCut(d, x, (int16_t)(y + Ui::Pitch), value + next, room);
            return 2;
        }
    } // namespace

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

    // ---- Pages ------------------------------------------------------------------
    void WidgetSysInfo::setPage(size_t page)
    {
        if (page >= PAGE_COUNT) page = PAGE_COUNT - 1;
        if (page != _page)
        {
            _page = page;
            _forceRedraw = true;
        }
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
        _forceRedraw = true;
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
            _forceRedraw = true;
        }
    }

    void WidgetSysInfo::loop()
    {
        if (_state != WidgetState::RUNNING || !_display || !_display->display) return;

        // Non-blocking: redraw once per second for the live values, at once after a page change.
        const uint32_t now = millis();
        if (_forceRedraw || now - _lastDraw >= REDRAW_INTERVAL_MS)
        {
            _lastDraw = now;
            _forceRedraw = false;
            drawPage();
        }
    }

    // ---- Drawing ----------------------------------------------------------------
    void WidgetSysInfo::drawPage()
    {
        Adafruit_SSD1306 &d = *_display->display;
        Ui::begin(d);
        d.clearDisplay();

        switch (_page)
        {
            case PAGE_DEVICE: drawDevice(d); break;
            case PAGE_FIRMWARE: drawFirmware(d); break;
            case PAGE_RUNTIME: drawRuntime(d); break;
            case PAGE_DIAGNOSTICS: drawDiagnostics(d); break;
            default: drawCard(d); break;
        }

        _display->displayBuff();
    }

    /** @brief Check mark and "Running"; warning and "Restarted" while the watchdog reset counter is not 0. */
    void WidgetSysInfo::drawCard(Adafruit_SSD1306 &d)
    {
        char detail[24];
        snprintf(detail, sizeof(detail), "Up %s", openknx.logger.buildUptime().c_str());
        // Without the watchdog the counter sits in uninitialised RAM and is never written.
    #if defined(OPENKNX_WATCHDOG) && !defined(OPENKNX_DEBUGGER)
        const bool restarted = openknx.watchdog.resets() > 0;
    #else
        const bool restarted = false;
    #endif
        Ui::statusCard(d, drawChipSymbol, restarted ? Ui::kBadgeWarn : Ui::kBadgeOk, restarted ? "Restarted" : "Running", detail);
        Ui::pageDots(d, PAGE_COUNT, PAGE_CARD);
    }

    void WidgetSysInfo::drawDevice(Adafruit_SSD1306 &d)
    {
        Ui::header(d, "Device", PAGE_COUNT, PAGE_DEVICE);
        uint8_t r = rowWrap(d, 0, "ID", openknx.info.firmwareName().c_str(), 3);
        Ui::row(d, r++, "SN", openknx.info.humanSerialNumber().c_str(), 3);
        Ui::row(d, r, "PA", openknx.info.humanIndividualAddress().c_str(), 3);
    }

    void WidgetSysInfo::drawFirmware(Adafruit_SSD1306 &d)
    {
        Ui::header(d, "Firmware", PAGE_COUNT, PAGE_FIRMWARE);
        Ui::row(d, 0, "Version", openknx.info.humanFirmwareVersion().c_str(), 8);
        Ui::row(d, 1, "Number", openknx.info.humanFirmwareNumber().c_str(), 8);
        Ui::row(d, 2, "KNX", knxTypeLabel(MASK_VERSION).c_str(), 8);
    }

    void WidgetSysInfo::drawRuntime(Adafruit_SSD1306 &d)
    {
        Ui::header(d, "Runtime", PAGE_COUNT, PAGE_RUNTIME);
        char b[24];
        snprintf(b, sizeof(b), "%.1f KiB", (float)freeMemory() / 1024.0f);
        Ui::row(d, 0, "Free", b, 7);

        // PSRAM only exists on ESP32 boards that have it: elsewhere the row is left out and Uptime moves up.
        uint8_t r = 1;
    #if defined(ARDUINO_ARCH_ESP32) && defined(BOARD_HAS_PSRAM)
        snprintf(b, sizeof(b), "%.0f KiB", (float)ESP.getFreePsram() / 1024.0f);
        Ui::row(d, r++, "PSRAM", b, 7);
    #endif
        Ui::row(d, r, "Uptime", openknx.logger.buildUptime().c_str(), 7);
    }

    void WidgetSysInfo::drawDiagnostics(Adafruit_SSD1306 &d)
    {
        Ui::header(d, "Diagnostics", PAGE_COUNT, PAGE_DIAGNOSTICS);
        char b[24];

        // The low-water mark, not the current value: a leak shows there.
        snprintf(b, sizeof(b), "%.1f KiB", (float)openknx.common.freeMemoryMin() / 1024.0f);
        Ui::row(d, 0, "Heap min", b, 9);

    #ifdef OPENKNX_DUALCORE
        snprintf(b, sizeof(b), "%i/%i B", openknx.common.freeStackMin(), openknx.common.freeStackMin1());
    #else
        snprintf(b, sizeof(b), "%i B", openknx.common.freeStackMin());
    #endif
        Ui::row(d, 1, "Stack", b, 9);

        // Consecutive watchdog restarts and the reset period.
        if (openknx.watchdog.active())
            snprintf(b, sizeof(b), "%u (%us)", (unsigned)openknx.watchdog.resets(), (unsigned)openknx.watchdog.maxPeriod());
        else
            snprintf(b, sizeof(b), "off");
        Ui::row(d, 2, "Watchdog", b, 9);

    #if defined(ARDUINO_ARCH_ESP32)
        snprintf(b, sizeof(b), "%s %u MHz", ESP.getChipModel(), (unsigned)getCpuFrequencyMhz());
    #elif defined(ARDUINO_ARCH_RP2040)
        snprintf(b, sizeof(b), "RP %u MHz", (unsigned)(F_CPU / 1000000UL));
    #else
        snprintf(b, sizeof(b), "-");
    #endif
        Ui::row(d, 3, "Chip", b, 9);
    }

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE
