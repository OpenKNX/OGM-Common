/**
 * @file        WidgetKnxBcu.cpp
 * @brief       Multi-page KNX / BCU status widget provided by OGM-Common
 * @version     0.0.1
 * @date        2026-07-11
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#ifdef DEVICE_DISPLAY_MODULE
    #include "WidgetKnxBcu.h"
    #include "OpenKNX.h"
    #include "knx.h" // knx.bau(), TpUartDataLinkLayer::getTPUart(), TPUart::Statistics

namespace OpenKNX
{
    namespace
    {
        std::string uStr(unsigned long v)
        {
            char b[16];
            snprintf(b, sizeof(b), "%lu", v);
            return std::string(b);
        }

        // Auto-range: smallest "nice" full-scale >= v. The bar scale ratchets up
        // through these steps (never shrinks) so small loads are visible early and
        // the peak stays readable (mock: 50/100/200/300/500/750/1000...).
        uint32_t niceScale(uint32_t v)
        {
            static const uint32_t steps[] = {50, 100, 200, 300, 500, 750, 1000, 1500, 2000, 3000, 5000, 10000};
            for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i)
                if (steps[i] >= v) return steps[i];
            return v; // beyond the table: use the raw value
        }
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
    // 0x091A (IP-Router coupler): the BCU sits on the SECONDARY (TP) DLL.
    // 0x07B0 (pure TP device):    the BCU is the PRIMARY DLL.
    // anything else:              no TP-UART BCU -> nullptr (widget shows "-").
    TpUartDataLinkLayer *WidgetKnxBcu::bcuDll() const
    {
    #if MASK_VERSION == 0x07B0
        return knx.bau().getDataLinkLayer();
    #elif MASK_VERSION == 0x091A
        return knx.bau().getSecondaryDataLinkLayer();
    #else
        return nullptr;
    #endif
    }

    // ---- Multi-page ------------------------------------------------------------
    void WidgetKnxBcu::setPage(size_t page)
    {
        if (page >= PAGE_COUNT) page = PAGE_COUNT - 1;
        _currentPage = page;
        _lastPageFlip = millis();
        _lastUpdate = 0; // redraw immediately
    }
    void WidgetKnxBcu::nextPage()
    {
        _currentPage = (_currentPage + 1) % PAGE_COUNT;
        _lastPageFlip = millis();
        _lastUpdate = 0;
    }
    void WidgetKnxBcu::prevPage()
    {
        _currentPage = (_currentPage + PAGE_COUNT - 1) % PAGE_COUNT;
        _lastPageFlip = millis();
        _lastUpdate = 0;
    }
    uint32_t WidgetKnxBcu::pageDwellMs() const
    {
        uint32_t dwell = (_displayTime > 0) ? (_displayTime / PAGE_COUNT) : 1000;
        if (dwell < 800) dwell = 800; // keep pages readable
        return dwell;
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
        _currentPage = 0;
        _startTime = millis();
        _lastPageFlip = _startTime;
        _lastUpdate = 0;
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
            _lastUpdate = 0;
        }
    }
    void WidgetKnxBcu::loop()
    {
        if (_state != WidgetState::RUNNING || !_display) return;
        uint32_t now = millis();

        // Internal auto-paging: cycle pages during the dwell. Pause freezes EVERYTHING —
        // including this internal page advance (the manager's updateAutoPaging is already
        // pause-gated, so the internal timer must be too; "Pause ist Pause").
        bool paused = false;
        if (openknxDisplayModule.getWidgetManager())
            paused = openknxDisplayModule.getWidgetManager()->isRotationPaused();

        if (!paused && (uint32_t)(now - _lastPageFlip) >= pageDwellMs())
        {
            _currentPage = (_currentPage + 1) % PAGE_COUNT;
            _lastPageFlip = now;
            _lastUpdate = 0;
        }
        else if (paused)
        {
            _lastPageFlip = now; // don't accumulate while paused -> no page jump on resume
        }

        // Refresh live values ~1x/s.
        if (_lastUpdate == 0 || (uint32_t)(now - _lastUpdate) >= 1000)
        {
            _lastUpdate = now;
            draw();
        }
    }

    // ---- Auto-range load bar bookkeeping ---------------------------------------
    void WidgetKnxBcu::updateLoadRatchet()
    {
        TpUartDataLinkLayer *dll = bcuDll();
        _lastLoad = dll ? (uint32_t)dll->getTPUart().getStatistics().getBusLoad() : 0;
        if (_lastLoad > _loadPeak) _loadPeak = _lastLoad;
        uint32_t needed = niceScale(_loadPeak);
        if (needed > _loadScale) _loadScale = needed; // ratchet up only (no decay)
    }

    // ---- Drawing ---------------------------------------------------------------
    void WidgetKnxBcu::draw()
    {
        if (!_display || !_display->display) return;

        updateLoadRatchet();
        TpUartDataLinkLayer *dll = bcuDll();

        _display->display->clearDisplay();
        _display->display->setTextColor(WHITE);
        _display->display->setTextSize(1);
        _display->display->setTextWrap(false);

        drawHeader();
        drawPageBody(dll);

        _display->displayBuff();
    }

    void WidgetKnxBcu::drawHeader()
    {
        const uint16_t w = _display->GetDisplayWidth();

        const std::string &title = getName();
        _display->display->setCursor((int16_t)((w - (title.length() * 6)) / 2), 0);
        _display->display->print(title.c_str());

        // Countdown marker running along the divider = widget dwell progress.
        if (_displayTime > 0)
        {
            uint32_t elapsed = millis() - _startTime;
            if (elapsed > _displayTime) elapsed = _displayTime;
            uint16_t markerX = (uint16_t)(((uint32_t)w * elapsed) / _displayTime);
            _display->display->fillCircle(markerX, 10, 2, WHITE);
            _display->display->drawCircle(markerX, 10, 2, BLACK);
        }
        _display->display->drawLine(0, 10, w, 10, WHITE);

        // Page indicator "n/N" top-right.
        char pg[8] = {};
        snprintf(pg, sizeof(pg), "%u/%u", (unsigned)(_currentPage + 1), (unsigned)PAGE_COUNT);
        _display->display->setCursor((int16_t)(w - (strlen(pg) * 6)), 0);
        _display->display->print(pg);
    }

    // Label left, value right-aligned (mock: "Texte linksbündig, Werte rechts").
    void WidgetKnxBcu::drawKeyValueRow(uint8_t row, const char *key, const std::string &value)
    {
        const int16_t y = (int16_t)(15 + row * 12);
        const uint16_t w = _display->GetDisplayWidth();

        _display->display->setCursor(0, y);
        _display->display->print(key);

        int16_t vx = (int16_t)(w - value.length() * 6);
        const int16_t minX = (int16_t)(strlen(key) * 6 + 6); // never overlap the label
        if (vx < minX) vx = minX;
        _display->display->setCursor(vx, y);
        _display->display->print(value.c_str());
    }

    // Auto-range bar with peak-hold marker + right-side scale number.
    void WidgetKnxBcu::drawLoadBar(int16_t y)
    {
        const int16_t barX = 0, barW = 92, barH = 9;
        _display->display->drawRect(barX, y, barW, barH, WHITE);

        const uint32_t scale = _loadScale ? _loadScale : 1;
        const uint32_t load = _lastLoad > scale ? scale : _lastLoad;
        const int16_t fillW = (int16_t)((uint32_t)(barW - 2) * load / scale);
        if (fillW > 0) _display->display->fillRect(barX + 1, y + 1, fillW, barH - 2, WHITE);

        // Peak-hold marker (peak >= load, so it sits at/right of the fill edge).
        const uint32_t pk = _loadPeak > scale ? scale : _loadPeak;
        int16_t peakX = (int16_t)(barX + 1 + (uint32_t)(barW - 2) * pk / scale);
        if (peakX > barX + barW - 2) peakX = (int16_t)(barX + barW - 2);
        _display->display->drawFastVLine(peakX, y, barH, WHITE);

        char sc[12] = {};
        snprintf(sc, sizeof(sc), "%lu", (unsigned long)scale);
        _display->display->setCursor((int16_t)(barX + barW + 4), (int16_t)(y + 1));
        _display->display->print(sc);
    }

    void WidgetKnxBcu::drawPageBody(TpUartDataLinkLayer *dll)
    {
        const bool ok = (dll != nullptr);
        switch (_currentPage)
        {
            case 0: // Stats-1: TX/RX frames, Load, Peak + auto-range bar
            {
                if (ok)
                {
                    auto &st = dll->getTPUart().getStatistics();
                    drawKeyValueRow(0, "TX/RX", uStr(st.getTxFrames()) + "/" + uStr(st.getRxFrames()));
                    drawKeyValueRow(1, "Load", uStr(_lastLoad) + " B/s");
                    drawKeyValueRow(2, "Peak", uStr(_loadPeak) + " B/s");
                }
                else
                {
                    drawKeyValueRow(0, "TX/RX", "-");
                    drawKeyValueRow(1, "Load", "-");
                    drawKeyValueRow(2, "Peak", "-");
                }
                drawLoadBar(52);
                break;
            }
            case 1: // Stats-2: Discarded/Received bytes, Await/Rep, Overflow
            {
                if (ok)
                {
                    auto &st = dll->getTPUart().getStatistics();
                    drawKeyValueRow(0, "Discard", uStr(st.getRxDiscardedBytes()) + " B");
                    drawKeyValueRow(1, "Receiv", uStr(st.getRxReceivedBytes()) + " B");
                    drawKeyValueRow(2, "Await/Rep",
                                    uStr(dll->getTPUart().getReceiver().getAwaitBytes()) + "/" + uStr(st.getRxRepetitions()));
                    drawKeyValueRow(3, "Overflow",
                                    uStr(st.getRxUartOverflow()) + "/" + uStr(st.getRxSearchBufferOverflow()) + "/" +
                                        uStr(st.getRxFrameBufferOverflow()) + "/" + uStr(st.getTxOverflowFrameBuffer()));
                }
                else
                {
                    drawKeyValueRow(0, "Discard", "-");
                    drawKeyValueRow(1, "Receiv", "-");
                    drawKeyValueRow(2, "Await/Rep", "-");
                    drawKeyValueRow(3, "Overflow", "-");
                }
                break;
            }
            case 2: // Status: BCU connection, baudrate, individual address
            {
                const std::string status = ok ? std::string(dll->getTPUart().getBcuStateInfo()) : std::string("-");
                std::string baud = "-";
                if (ok && dll->getTPUart().getBaudrate() > 0)
                    baud = uStr((unsigned long)dll->getTPUart().getBaudrate());
                drawKeyValueRow(0, "Status", status);
                drawKeyValueRow(1, "Baud", baud);
                drawKeyValueRow(2, "Adresse", openknx.info.humanIndividualAddress());
                break;
            }
    #ifdef TPUART_BCU_HEALTH
            case 3: // Health
            {
                if (ok)
                {
                    auto &st = dll->getTPUart().getStatistics();
                    drawKeyValueRow(0, "Resets", uStr(st.getBcuResets()));
                    drawKeyValueRow(1, "Disconn", uStr(st.getBcuDisconnects()));
                    drawKeyValueRow(2, "CON-resc", uStr(st.getBcuConRescues()));
                }
                else
                {
                    drawKeyValueRow(0, "Resets", "-");
                    drawKeyValueRow(1, "Disconn", "-");
                    drawKeyValueRow(2, "CON-resc", "-");
                }
                break;
            }
            case 4: // NCN-Err
            {
                if (ok)
                {
                    auto &st = dll->getTPUart().getStatistics();
                    drawKeyValueRow(0, "SlaveColl", uStr(st.getBcuSlaveCollisions()));
                    drawKeyValueRow(1, "Recv-Err", uStr(st.getBcuReceiveErrors()));
                    drawKeyValueRow(2, "Tx/Proto", uStr(st.getBcuTransmitErrors()) + "/" + uStr(st.getBcuProtocolErrors()));
                    drawKeyValueRow(3, "TempWarn", uStr(st.getBcuTempWarnings()));
                }
                else
                {
                    drawKeyValueRow(0, "SlaveColl", "-");
                    drawKeyValueRow(1, "Recv-Err", "-");
                    drawKeyValueRow(2, "Tx/Proto", "-");
                    drawKeyValueRow(3, "TempWarn", "-");
                }
                break;
            }
    #endif
            default: break;
        }
    }

} // namespace OpenKNX

#endif // DEVICE_DISPLAY_MODULE
