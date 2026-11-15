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

    // Subclasses the DeviceDisplay Widget base (OFM-DeviceDisplay); the bare "Widget.h" / "DeviceDisplay.h"
    // includes resolve via the OFM-DeviceDisplay src/ include path.
    //
    // Pages (the WidgetsManager rotates and flips them; without auto paging the rotation shows page 0):
    //   0  Card     Bus symbol · badge · "Bus OK" / "Bus lost" / "Starting" / "Busmon" / "No bus" (no TP link) · load
    //   1  Traffic  Status · TX/RX frames · Load · Peak + auto-range load bar
    //   2  Bytes    Discarded/received bytes · Await · Repetitions · Overflows
    //   3  Status   BCU connection (getBcuStateInfo) · Baud rate · Address
    //   4  Health   Resets · Disconnects · CON rescues                      } only with TPUART_BCU_HEALTH
    //   5  NCN      Slave collisions · Recv/Tx/Proto errors · Temp warnings }
    //
    // BCU values come live from the TP-UART data-link layer (KNX_TP_DLL), shown as "-" without a DLL; load and
    // peak come from openknx.busLoad, the address from openknx.info.
    #include "DeviceDisplay.h"
    #include "Widget.h"

class TpUartDataLinkLayer; // knx global type; full def pulled in the .cpp only

namespace OpenKNX
{
    /**
     * @brief Carries a 32-bit statistics counter past its wrap point for display.
     *
     * The TP-UART counters are `unsigned int` and wrap at 4294967295; the byte counters reach that after
     * 14 to 28 days at the host UART limit (38400 or 19200 baud). Widening them in TPUart would put
     * non-atomic 64-bit increments into the RX hot path, so the wrap is carried here. A drop counts as a
     * wrap when the counter could have risen past the wrap since the last sample (maxRise), else as a
     * counter reset (TP-UART reset), after which the value counts from that reset like its source. A reset
     * after a gap long enough for a wrap counts as a wrap. Not seen: a wrap in a gap after which the counter
     * is above its last sample again, a second wrap in one gap, and a gap of 49.7 days or more (millis()).
     */
    struct WidgetCounterWrap
    {
        uint32_t last = 0;
        uint16_t wraps = 0;

        uint64_t update(uint32_t value, uint32_t maxRise)
        {
            if (value < last) wraps = (value - last <= maxRise) ? static_cast<uint16_t>(wraps + 1) : 0; // value - last: rise past the wrap
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

        // Pages: health pages only with TPUART_BCU_HEALTH.
        size_t getPageCount() const override { return PAGE_COUNT; }
        size_t getCurrentPage() const override { return _page; }
        void setPage(size_t page) override;
        void nextPage() override { setPage((_page + 1) % PAGE_COUNT); }
        void prevPage() override { setPage((_page + PAGE_COUNT - 1) % PAGE_COUNT); }

      private:
        enum Page : uint8_t
        {
            PAGE_CARD = 0,
            PAGE_TRAFFIC,
            PAGE_BYTES,
            PAGE_STATUS,
    #ifdef TPUART_BCU_HEALTH
            PAGE_HEALTH,
            PAGE_NCN,
    #endif
            PAGE_COUNT
        };

        static constexpr uint32_t REDRAW_INTERVAL_MS = 1000;
        static constexpr uint32_t CARD_REDRAW_MS = 250; // card page: the bus devices light up with traffic

        WidgetState _state;
        uint32_t _displayTime;
        WidgetFlags _action;
        i2cDisplay *_display;
        std::string _name = "KNX / BCU"; // rotation settings are stored by this name: never rename it

        size_t _page = 0;
        uint32_t _lastDraw = 0;
        bool _forceRedraw = true;
        uint8_t _blink = 0;       // running-light step of the card symbol
        uint32_t _lastFrames = 0; // TX + RX frames at the last card draw (low 32 bits)
        bool _cardShown = false;  // the last frame was the card: only then does a counter change light a device

        // Auto-range load bar: full-scale ratchets UP through nice steps, never shrinks; a marker holds the peak.
        uint16_t _loadScale = 50; // current full-scale (B/s), starts small
        uint16_t _loadPeak = 0;   // running peak load (B/s)
        uint16_t _lastLoad = 0;   // last sampled bus load (B/s)

        // Wrap-carrying samples of the four counters that can realistically reach 2^32, plus their
        // last sampled value. Sampled once per redraw for ALL pages, not only the visible one.
        WidgetCounterWrap _wrapTxFrames, _wrapRxFrames, _wrapDiscarded, _wrapReceived;
        uint32_t _sampleAt = 0; // millis() of the last counter sample
        uint64_t _txFrames = 0, _rxFrames = 0, _discardedBytes = 0, _receivedBytes = 0;

        // TP-UART DLL for this device (secondary on 0x091A, primary on 0x07B0), or nullptr before it exists.
        TpUartDataLinkLayer *bcuDll() const;

        void updateLoadRatchet();                      // sample bus load, grow peak + scale
        void sampleCounters(TpUartDataLinkLayer *dll); // carry the wrapping 32-bit counters
        void draw();
        void drawCard(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll);
        void drawTraffic(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll);
        void drawBytes(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll);
        void drawStatus(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll);
    #ifdef TPUART_BCU_HEALTH
        void drawHealth(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll);
        void drawNcn(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll);
    #endif
        void drawLoadBar(Adafruit_SSD1306 &d, int16_t y);
    };

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE && KNX_HAS_TP
