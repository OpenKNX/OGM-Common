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

// Lives in OGM-Common but subclasses the DeviceDisplay Widget base (OFM-DeviceDisplay); PlatformIO puts
// that library's src/ on the include path, so the bare includes resolve like in OFM-SDCard.
#include "Widget.h"
#include "DeviceDisplay.h"

namespace OpenKNX
{
    /**
     * @brief Status card plus four expert pages (device, firmware, runtime, diagnostics).
     * The WidgetsManager rotates and flips the pages; the widget only draws what the OpenKNX facades hold.
     */
    class WidgetSysInfo : public Widget
    {
      public:
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

        // Pages: page 0 is the card; without auto paging the rotation shows only page 0.
        size_t getPageCount() const override { return PAGE_COUNT; }
        size_t getCurrentPage() const override { return _page; }
        void setPage(size_t page) override;
        void nextPage() override { setPage((_page + 1) % PAGE_COUNT); }
        void prevPage() override { setPage((_page + PAGE_COUNT - 1) % PAGE_COUNT); }

        // KNX type label derived from the KNX MASK_VERSION.
        //   0x091A -> "Router 091A", 0x07B0 -> "TP Device 07B0",
        //   0x57B0 -> "Coupler 57B0", else "<HEX>".
        static std::string knxTypeLabel(uint16_t maskVersion);

      private:
        enum Page : uint8_t
        {
            PAGE_CARD = 0,
            PAGE_DEVICE,
            PAGE_FIRMWARE,
            PAGE_RUNTIME,
            PAGE_DIAGNOSTICS,
            PAGE_COUNT
        };

        static constexpr uint32_t REDRAW_INTERVAL_MS = 1000;

        WidgetState _state;
        uint32_t _displayTime;
        WidgetFlags _action;
        i2cDisplay *_display;
        std::string _name = "System-Info"; // rotation settings are stored by this name: never rename it
        size_t _page = 0;
        uint32_t _lastDraw = 0;
        bool _forceRedraw = true;

        void drawPage();
        void drawCard(Adafruit_SSD1306 &d);
        void drawDevice(Adafruit_SSD1306 &d);
        void drawFirmware(Adafruit_SSD1306 &d);
        void drawRuntime(Adafruit_SSD1306 &d);
        void drawDiagnostics(Adafruit_SSD1306 &d);
    };

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE
