/**
 * @file        WidgetKnxBcu.cpp
 * @brief       Multi-page KNX / BCU status widget provided by OGM-Common
 * @version     0.0.1
 * @date        2026-07-11
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#include "knx/config.h"
#if defined(DEVICE_DISPLAY_MODULE) && defined(KNX_HAS_TP)
    #include "WidgetKnxBcu.h"
    #include "DisplayUi.h"
    #include "Icons/UiIcons.h"
    #include "OpenKNX.h"
    #include "knx.h" // knx.bau(), TpUartDataLinkLayer::getTPUart(), TPUart::Statistics

namespace OpenKNX
{
    namespace
    {
        // Auto-range: smallest "nice" full-scale >= v. The bar scale ratchets up
        // through these steps (never shrinks) so small loads are visible early and
        // the peak stays readable.
        uint16_t niceScale(uint16_t v)
        {
            static const uint16_t steps[] = {50, 100, 200, 300, 500, 750, 1000, 1500, 2000, 3000, 5000, 10000};
            for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i)
                if (steps[i] >= v) return steps[i];
            return v; // beyond the table: use the raw value
        }

        /** @brief Row icon of an error counter: check mark at 0, warning triangle otherwise. */
        const uint8_t *rowState(uint64_t count) { return count ? Ui::kRowWarn : Ui::kRowOk; }
    } // namespace

    WidgetKnxBcu::WidgetKnxBcu(uint32_t displayTime, WidgetFlags action)
        : _state(WidgetState::STOPPED), _displayTime(displayTime), _action(action), _display(nullptr)
    {
    }

    // ---- Widget getters/setters -------------------------------------------------
    uint32_t WidgetKnxBcu::getDisplayTime() const { return _displayTime; }
    WidgetFlags WidgetKnxBcu::getAction() const { return _action; }

    void WidgetKnxBcu::setDisplayModule(i2cDisplay *displayModule)
    {
        _display = displayModule;
        if (_display == nullptr) logErrorP("Display is NULL.");
    }
    i2cDisplay *WidgetKnxBcu::getDisplayModule() const { return _display; }

    // ---- Coupler-aware DLL accessor --------------------------------------------
    // 0x091A (IP-Router coupler): the BCU sits on the SECONDARY (TP) DLL; 0x07B0 (TP device): the PRIMARY DLL.
    TpUartDataLinkLayer *WidgetKnxBcu::bcuDll() const
    {
        return KNX_TP_DLL;
    }

    // ---- Pages ---------------------------------------------------------------
    void WidgetKnxBcu::setPage(size_t page)
    {
        if (page >= PAGE_COUNT) page = PAGE_COUNT - 1;
        if (page != _page)
        {
            _page = page;
            _forceRedraw = true;
        }
    }

    // ---- Lifecycle -------------------------------------------------------------
    void WidgetKnxBcu::setup()
    {
        if (_display == nullptr) logErrorP("Display is NULL.");
    }
    void WidgetKnxBcu::start()
    {
        if (_state == WidgetState::RUNNING) return;
        _state = WidgetState::RUNNING;
        _forceRedraw = true;
        _cardShown = false; // the first card frame only records the counters
    }
    void WidgetKnxBcu::stop()
    {
        _state = WidgetState::STOPPED;
        if (_display && _display->display)
        {
            _display->display->clearDisplay();
            _display->displayBuff();
        }
    }
    void WidgetKnxBcu::pause()
    {
        if (_state == WidgetState::RUNNING) _state = WidgetState::PAUSED;
    }
    void WidgetKnxBcu::resume()
    {
        if (_state == WidgetState::PAUSED)
        {
            _state = WidgetState::RUNNING;
            _forceRedraw = true;
            _cardShown = false;
        }
    }
    void WidgetKnxBcu::loop()
    {
        if (_state != WidgetState::RUNNING || !_display || !_display->display) return;

        // Non-blocking: refresh the live values once per second (the card 4x for its traffic light),
        // at once after a page change.
        const uint32_t now = millis();
        const uint32_t interval = _page == PAGE_CARD ? CARD_REDRAW_MS : REDRAW_INTERVAL_MS;
        if (_forceRedraw || now - _lastDraw >= interval)
        {
            _lastDraw = now;
            _forceRedraw = false;
            draw();
        }
    }

    // ---- Auto-range load bar bookkeeping ---------------------------------------
    void WidgetKnxBcu::updateLoadRatchet()
    {
        _lastLoad = openknx.busLoad.currentBytesPerSec();
        _loadPeak = openknx.busLoad.peakBytesPerSec();
        const uint16_t needed = niceScale(_loadPeak);
        if (needed > _loadScale) _loadScale = needed; // ratchet up only (no decay)
    }

    // ---- Wrap-carrying counter sampling ----------------------------------------
    void WidgetKnxBcu::sampleCounters(TpUartDataLinkLayer *dll)
    {
        if (dll == nullptr) return;

        auto &st = dll->getTPUart().getStatistics();
        _txFrames = _wrapTxFrames.update(st.getTxFrames());
        _rxFrames = _wrapRxFrames.update(st.getRxFrames());
        _discardedBytes = _wrapDiscarded.update(st.getRxDiscardedBytes());
        _receivedBytes = _wrapReceived.update(st.getRxReceivedBytes());
    }

    // ---- Drawing ---------------------------------------------------------------
    void WidgetKnxBcu::draw()
    {
        updateLoadRatchet();
        TpUartDataLinkLayer *dll = bcuDll();
        sampleCounters(dll);

        Adafruit_SSD1306 &d = *_display->display;
        Ui::begin(d);
        d.clearDisplay();

        if (_page != PAGE_CARD) _cardShown = false; // traffic meanwhile must not light a device on return
        switch (_page)
        {
            case PAGE_TRAFFIC: drawTraffic(d, dll); break;
            case PAGE_BYTES: drawBytes(d, dll); break;
            case PAGE_STATUS: drawStatus(d, dll); break;
    #ifdef TPUART_BCU_HEALTH
            case PAGE_HEALTH: drawHealth(d, dll); break;
            case PAGE_NCN: drawNcn(d, dll); break;
    #endif
            default: drawCard(d, dll); break;
        }

        _display->displayBuff();
    }

    // Frame 10 px high with the fill 2 px inside, peak-hold marker, full-scale number right of it.
    void WidgetKnxBcu::drawLoadBar(Adafruit_SSD1306 &d, int16_t y)
    {
        constexpr int16_t barW = 92, inner = barW - 2 * Ui::BarPad;
        const uint16_t scale = _loadScale ? _loadScale : 1;
        const uint16_t load = _lastLoad > scale ? scale : _lastLoad;
        Ui::bar(d, 0, y, barW, (uint16_t)((uint32_t)load * 1000 / scale));

        // Peak marker inside the frame (peak >= load, so at or right of the fill edge); none while the peak is 0.
        if (_loadPeak > 0)
        {
            const uint16_t pk = _loadPeak > scale ? scale : _loadPeak;
            int16_t px = (int16_t)(Ui::BarPad + (uint32_t)inner * pk / scale);
            if (px > Ui::BarPad + inner - 1) px = (int16_t)(Ui::BarPad + inner - 1);
            d.drawFastVLine(px, (int16_t)(y + 1), Ui::BarH - 2, WHITE);
        }

        char sc[8]; // scale <= 65535
        snprintf(sc, sizeof(sc), "%u", (unsigned)scale);
        d.setCursor((int16_t)(barW + 4), (int16_t)(y + 1));
        d.print(sc);
    }

    // Device lit by the traffic light of the card, -1 = none; set right before statusCard() draws the symbol.
    static int8_t s_litDevice = -1;

    /** @brief Card symbol: three devices on a two-wire bus line in x40..79 y2..23, clear of six page dots from x80. */
    static void drawBusSymbol(Adafruit_SSD1306 &d)
    {
        int8_t i = 0;
        for (int16_t x = 42; x <= 68; x += 13, i++)
        {
            if (i == s_litDevice)
                d.fillRoundRect(x, 2, 11, 9, 1, WHITE);
            else
                d.drawRoundRect(x, 2, 11, 9, 1, WHITE);
            d.drawFastVLine((int16_t)(x + 5), 11, 6, WHITE);
        }
        d.fillRect(40, 17, 40, 2, WHITE);
        d.fillRect(40, 22, 40, 2, WHITE);
    }

    /** @brief Page 0, shown by the rotation: bus state as a card, the load as detail. */
    void WidgetKnxBcu::drawCard(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll)
    {
        const uint8_t *badge = Ui::kBadgeError;
        const char *word = "Bus lost";
        const char *detail = "Check KNX line";
        char b[24];
        if (!dll)
        {
            word = "No bus";
            detail = "No TP link";
        }
        else
        {
            TPUart::DataLinkLayer &tp = dll->getTPUart();
            switch (tp.getBcuState())
            {
                case TPUart::BCU_UNINITIALIZED:
                    if (uptime() < 30) // only the first 30 s after boot count as start-up (wrap-safe seconds)
                    {
                        badge = Ui::kBadgeClock;
                        word = "Starting";
                        detail = "TP starting";
                    }
                    break;
                case TPUart::BCU_BUSMONITOR:
                    badge = nullptr;
                    word = "Busmon";
                    detail = "Monitor active";
                    break;
                case TPUart::BCU_CONNECTED:
                    if (tp.busOperational()) // bus power loss shows here before the BCU state follows
                    {
                        snprintf(b, sizeof(b), "Load %s/s", humanBytes(_lastLoad).c_str());
                        badge = Ui::kBadgeOk;
                        word = "Bus OK";
                        detail = b;
                    }
                    break;
                default: break; // disconnected: "Bus lost"
            }
        }
        // Traffic light: a new frame since the last card draw lights the next device, no traffic none.
        const uint32_t frames = (uint32_t)(_txFrames + _rxFrames);
        s_litDevice = -1;
        if (dll && _cardShown && frames != _lastFrames)
        {
            _blink = (uint8_t)((_blink + 1) % 3);
            s_litDevice = (int8_t)_blink;
        }
        _lastFrames = frames;
        _cardShown = true;
        Ui::statusCard(d, drawBusSymbol, badge, word, detail);
        Ui::pageDots(d, PAGE_COUNT, PAGE_CARD);
    }

    /** @brief Page 1: BCU state, frames, load, peak and the load bar. */
    void WidgetKnxBcu::drawTraffic(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll)
    {
        Ui::header(d, _name.c_str(), PAGE_COUNT, PAGE_TRAFFIC);
        char b[24];
        Ui::row(d, 0, "Status", dll ? dll->getTPUart().getBcuStateInfo() : "-", 7);
        if (dll)
            snprintf(b, sizeof(b), "%s/%s", humanCount(_txFrames).c_str(), humanCount(_rxFrames).c_str());
        else
            snprintf(b, sizeof(b), "-");
        Ui::row(d, 1, "TX/RX", b, 7);
        snprintf(b, sizeof(b), "%s/s", humanBytes(_lastLoad).c_str());
        Ui::row(d, 2, "Load", dll ? b : "-", 7);
        snprintf(b, sizeof(b), "%s/s", humanBytes(_loadPeak).c_str());
        Ui::row(d, 3, "Peak", dll ? b : "-", 7);
        drawLoadBar(d, 54);
    }

    /** @brief Page 2: byte counters and overflows; error counters with a check at 0, a warning otherwise. */
    void WidgetKnxBcu::drawBytes(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll)
    {
        Ui::header(d, "Bytes", PAGE_COUNT, PAGE_BYTES);
        if (!dll)
        {
            static const char *const kLabels[] = {"Discarded", "Received", "Await", "Repeated", "Overflow"};
            for (uint8_t i = 0; i < 5; i++) Ui::rowIcon(d, i, nullptr, kLabels[i], "-");
            return;
        }
        auto &st = dll->getTPUart().getStatistics();
        Ui::rowIcon(d, 0, rowState(_discardedBytes), "Discarded", humanBytes(_discardedBytes).c_str());
        Ui::rowIcon(d, 1, Ui::kRowBus, "Received", humanBytes(_receivedBytes).c_str());
        Ui::rowIcon(d, 2, Ui::kRowInfo, "Await", humanCount(dll->getTPUart().getReceiver().getAwaitBytes()).c_str());
        Ui::rowIcon(d, 3, Ui::kRowInfo, "Repeated", humanCount(st.getRxRepetitions()).c_str());

        // UART / search buffer / frame buffer / TX frame buffer: the short form keeps all four on one row.
        const uint32_t uart = st.getRxUartOverflow(), search = st.getRxSearchBufferOverflow();
        const uint32_t frame = st.getRxFrameBufferOverflow(), tx = st.getTxOverflowFrameBuffer();
        char b[24];
        snprintf(b, sizeof(b), "%s/%s/%s/%s", humanCountShort(uart).c_str(), humanCountShort(search).c_str(),
                 humanCountShort(frame).c_str(), humanCountShort(tx).c_str());
        Ui::rowIcon(d, 4, rowState((uint64_t)uart + search + frame + tx), "Overflow", b);
    }

    /** @brief Page 3: BCU connection, baud rate, individual address. */
    void WidgetKnxBcu::drawStatus(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll)
    {
        Ui::header(d, "Status", PAGE_COUNT, PAGE_STATUS);
        char b[12];
        if (dll && dll->getTPUart().getBaudrate() > 0)
            snprintf(b, sizeof(b), "%lu", (unsigned long)dll->getTPUart().getBaudrate());
        else
            snprintf(b, sizeof(b), "-");
        Ui::row(d, 0, "Status", dll ? dll->getTPUart().getBcuStateInfo() : "-", 8);
        Ui::row(d, 1, "Baud", b, 8);
        Ui::row(d, 2, "Address", openknx.info.humanIndividualAddress().c_str(), 8);
    }

    #ifdef TPUART_BCU_HEALTH
    /** @brief Page 4: BCU resets, disconnects, CON rescues. */
    void WidgetKnxBcu::drawHealth(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll)
    {
        Ui::header(d, "Health", PAGE_COUNT, PAGE_HEALTH);
        static const char *const kLabels[] = {"Resets", "Disconnects", "CON rescues"};
        uint32_t v[3] = {};
        if (dll)
        {
            auto &st = dll->getTPUart().getStatistics();
            v[0] = st.getBcuResets();
            v[1] = st.getBcuDisconnects();
            v[2] = st.getBcuConRescues();
        }
        for (uint8_t i = 0; i < 3; i++)
            Ui::rowIcon(d, i, dll ? rowState(v[i]) : nullptr, kLabels[i], dll ? humanCount(v[i]).c_str() : "-");
    }

    /** @brief Page 5: error counters of the NCN chip. */
    void WidgetKnxBcu::drawNcn(Adafruit_SSD1306 &d, TpUartDataLinkLayer *dll)
    {
        Ui::header(d, "NCN errors", PAGE_COUNT, PAGE_NCN);
        static const char *const kLabels[] = {"Slave coll.", "Recv errors", "Tx errors", "Proto errors", "Temp warnings"};
        uint32_t v[5] = {};
        if (dll)
        {
            auto &st = dll->getTPUart().getStatistics();
            v[0] = st.getBcuSlaveCollisions();
            v[1] = st.getBcuReceiveErrors();
            v[2] = st.getBcuTransmitErrors();
            v[3] = st.getBcuProtocolErrors();
            v[4] = st.getBcuTempWarnings();
        }
        for (uint8_t i = 0; i < 5; i++)
            Ui::rowIcon(d, i, dll ? rowState(v[i]) : nullptr, kLabels[i], dll ? humanCount(v[i]).c_str() : "-");
    }
    #endif

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE && KNX_HAS_TP
