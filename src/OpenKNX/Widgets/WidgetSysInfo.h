/**
 * @file        WidgetSysInfo.h
 * @brief       Multi-page system-information widget provided by OGM-Common
 * @version     0.0.1
 * @date        2026-07-11
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#ifdef DEVICE_DISPLAY_MODULE
#pragma once

// Multi-page system-information widget provided by OGM-Common.
// Lives in OGM-Common but subclasses the DeviceDisplay Widget base (OFM-DeviceDisplay).
// The Widget base + i2cDisplay come from OFM-DeviceDisplay; PlatformIO puts that
// library's src/ on the include path, so the bare "Widget.h" / "DeviceDisplay.h"
// includes resolve exactly like OFM-SDCard/src/Widgets/WidgetSDCard.h does.
#include "Widget.h"
#include "DeviceDisplay.h"

namespace OpenKNX
{
    // Four rotating pages built from the OpenKNX facades (openknx.info / freeMemory /
    // openknx.logger.buildUptime). Draw basis: header + countdown like
    // WidgetSDCard::drawSDInfo, a 4-line key/value body and an "n/4" page indicator.
    // Auto-paging derives the per-page dwell time from displayTime / pageCount.
    class WidgetSysInfo : public Widget
    {
      public:
        static constexpr size_t PAGE_COUNT = 4; // Gerät / Firmware / Runtime / Netzwerk

        const std::string logPrefix() { return "WidgetSysInfo"; }
        WidgetSysInfo(uint32_t displayTime, WidgetFlags action);

        // Core Widget interface
        void setup() override;
        void start() override;
        void stop() override;
        void pause() override;
        void resume() override;
        void loop() override;

        inline const WidgetState getState() const override { return _state; }
        inline const std::string getName() const override { return _name; }
        inline void setName(const std::string &name) override { _name = name; }

        uint32_t getDisplayTime() const override;
        WidgetFlags getAction() const override;

        inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }
        inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }
        inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }
        inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); }

        void setDisplayModule(i2cDisplay *displayModule) override;
        i2cDisplay *getDisplayModule() const override;

        // Multi-page interface
        size_t getPageCount() const override { return PAGE_COUNT; }
        size_t getCurrentPage() const override { return _currentPage; }
        void setPage(size_t page) override;
        void nextPage() override;
        void prevPage() override;

        // KNX type label derived from the KNX MASK_VERSION.
        //   0x091A -> "Router 091A", 0x07B0 -> "TP Device 07B0",
        //   0x57B0 -> "Coupler 57B0", else "<HEX>".
        static std::string knxTypeLabel(uint16_t maskVersion);

      private:
        WidgetState _state;
        uint32_t _displayTime;
        WidgetFlags _action;
        i2cDisplay *_display;
        std::string _name = "System-Info";

        size_t _currentPage = 0;      // 0..PAGE_COUNT-1
        uint32_t _lastUpdate = 0;     // last 1x/s redraw
        uint32_t _lastPageFlip = 0;   // auto-paging timer base
        uint32_t _startTime = 0;      // countdown base (like WidgetSDCard)

        // Auto-paging dwell per page = displayTime / PAGE_COUNT (min clamp).
        uint32_t pageDwellMs() const;

        void drawSysInfo();
        void drawHeader();                                   // header line + countdown + divider + "n/4"
        void drawPageBody();                                 // 4-line key/value body for the current page
        void drawKeyValueRow(uint8_t row, const char *key, const std::string &value);
    };

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE
