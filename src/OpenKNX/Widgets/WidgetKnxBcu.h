/**
 * @file        WidgetKnxBcu.h
 * @brief       Multi-page KNX / BCU status widget provided by OGM-Common
 * @version     0.0.1
 * @date        2026-07-11
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#include "knx/config.h"
#if defined(DEVICE_DISPLAY_MODULE) && defined(KNX_HAS_TP)
    #pragma once

    // WidgetKnxBcu — multi-page KNX / BCU status widget provided by OGM-Common.
    // Subclasses the DeviceDisplay Widget base (OFM-DeviceDisplay); the bare "Widget.h" /
    // "DeviceDisplay.h" includes resolve via the OFM-DeviceDisplay src/ include path.
    //
    // Pages (blätterbar mit Rauf/Runter, auto-rotierend während der Anzeigedauer):
    //   1  Stats   TX/RX Frames · Load (B/s) · Peak + Auto-Range-Load-Balken
    //   2  Stats   Discarded/Received Bytes · Await/Repetitions · Overflow
    //   3  Status  BCU-Verbindung (getBcuStateInfo) · Baudrate · Adresse
    //   4  Health  Resets · Disconnects · CON-rescues        } nur mit TPUART_BCU_HEALTH
    //   5  NCN-Err Slave-Coll · Recv/Tx/Proto-Err · Temp-Warn }
    //
    // All values are pulled LIVE from the TP-UART data-link layer: the SECONDARY DLL on
    // the 0x091A coupler (IP-Router), the PRIMARY DLL on the 0x07B0 TP device. On non-
    // coupler builds / before a DLL exists everything renders "-". Never random().
    #include "DeviceDisplay.h"
    #include "Widget.h"

class TpUartDataLinkLayer; // knx global type; full def pulled in the .cpp only

namespace OpenKNX
{
    /**
     * @brief Carries a 32-bit statistics counter past its wrap point for display.
     *
     * The TP-UART counters are `unsigned int` and wrap at 4294967295 - the byte counters reach that
     * after ~29 days of full TP1 load. Widening them in TPUart would put non-atomic 64-bit
     * increments into the RX hot path, so the wrap is carried here instead: each sample that comes
     * out lower than the previous one counts one wrap. Sampling only happens while the widget
     * itself is running (1x/s), so a wrap that occurs while it is disabled is not seen.
     */
    struct WidgetCounterWrap
    {
        uint32_t last = 0;
        uint16_t wraps = 0;

        uint64_t update(uint32_t value)
        {
            if (value < last) ++wraps;
            last = value;
            return ((uint64_t)wraps << 32) | value;
        }
    };

    class WidgetKnxBcu : public Widget
    {
      public:
        const std::string logPrefix() { return "WidgetKnxBcu"; }
        WidgetKnxBcu(uint32_t displayTime, WidgetFlags action);

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

        // Multi-page. Health + NCN-Err pages exist only with TPUART_BCU_HEALTH.
        size_t getPageCount() const override { return PAGE_COUNT; }
        size_t getCurrentPage() const override { return _currentPage; }
        void setPage(size_t page) override;
        void nextPage() override;
        void prevPage() override;

      private:
    #ifdef TPUART_BCU_HEALTH
        static constexpr size_t PAGE_COUNT = 5; // Stats1, Stats2, Status, Health, NCN-Err
    #else
        static constexpr size_t PAGE_COUNT = 3; // Stats1, Stats2, Status
    #endif

        WidgetState _state;
        uint32_t _displayTime;
        WidgetFlags _action;
        i2cDisplay *_display;
        std::string _name = "KNX / BCU";

        size_t _currentPage = 0;
        uint32_t _lastPageFlip = 0; // internal auto-paging timer (mirrors WidgetSysInfo)
        uint32_t _lastUpdate = 0;   // 1x/s redraw
        uint32_t _startTime = 0;    // countdown base

        // Auto-range load bar: full-scale ratchets UP through nice steps, never shrinks;
        // a peak-hold marker shows the maximum (see doc/menu-system BCU mock).
        uint32_t _loadScale = 50; // current full-scale (B/s), starts small
        uint32_t _loadPeak = 0;   // running peak load (B/s)
        uint32_t _lastLoad = 0;   // last sampled bus load (B/s)

        // Wrap-carrying samples of the four counters that can realistically reach 2^32, plus their
        // last sampled value. Sampled once per redraw for ALL pages, not only the visible one -
        // otherwise the page rotation would leave each counter unsampled most of the time.
        WidgetCounterWrap _wrapTxFrames, _wrapRxFrames, _wrapDiscarded, _wrapReceived;
        uint64_t _txFrames = 0, _rxFrames = 0, _discardedBytes = 0, _receivedBytes = 0;

        uint32_t pageDwellMs() const;

        // TP-UART DLL for this device (secondary on 0x091A, primary on 0x07B0), or
        // nullptr on non-coupler builds / before it exists.
        TpUartDataLinkLayer *bcuDll() const;

        void updateLoadRatchet();                          // sample bus load, grow peak + scale
        void sampleCounters(TpUartDataLinkLayer *dll);     // carry the wrapping 32-bit counters
        void draw();
        void drawHeader();
        void drawPageBody(TpUartDataLinkLayer *dll);
        void drawKeyValueRow(uint8_t row, const char *key, const std::string &value);
        void drawLoadBar(int16_t y);
    };

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE && KNX_HAS_TP
