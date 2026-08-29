#include "OpenKNX/BusLoad.h"
#include "OpenKNX.h"
#include "knx/config.h"
#ifdef KNX_HAS_TP
    #include "knx.h"
#endif

namespace OpenKNX
{
    void BusLoad::loop()
    {
#ifdef KNX_HAS_TP
        const uint32_t now = millis();
        if (_armed && (uint32_t)(now - _lastTick) < 1000)
            return;

        TpUartDataLinkLayer *dll = KNX_TP_DLL;
        if (dll == nullptr)
            return;

        TPUart::Statistics &st = dll->getTPUart().getStatistics();
        const uint32_t bits = st.getRxFrameBits();      // exact line time of every frame seen
        const uint32_t bytes = st.getRxBusBytes();      // frame + discarded octets, for the B/s readout
        const uint32_t discarded = st.getRxDiscardedBytes();

        // A BCU reset zeroes the counters: re-arm instead of underflowing the delta.
        if (!_armed || bits < _lastBits || bytes < _lastBytes || discarded < _lastDiscarded)
        {
            _armed = true;
            _lastTick = now;
            _lastBits = bits;
            _lastBytes = bytes;
            _lastDiscarded = discarded;
            return;
        }

        const uint32_t elapsed = now - _lastTick;
        const uint32_t deltaBits = bits - _lastBits;
        const uint32_t deltaBytes = bytes - _lastBytes;
        const uint32_t deltaDiscarded = discarded - _lastDiscarded;
        _lastTick = now;
        _lastBits = bits;
        _lastBytes = bytes;
        _lastDiscarded = discarded;

        if (elapsed == 0)
            return;

        const uint32_t perSec = (deltaBytes * 1000UL) / elapsed;
        _currentBytes = perSec > 0xFFFF ? (uint16_t)0xFFFF : (uint16_t)perSec;
        if (_currentBytes > _peakBytes)
            _peakBytes = _currentBytes;

        // Busy bits: the frames' cost plus octets that never formed one. 1000000/9600 = 625/6.
        const uint32_t occupied = deltaBits + deltaDiscarded * BIT_PER_OCTET;
        uint32_t permille = (occupied * 625UL) / (6UL * elapsed);
        if (permille > 0xFFFF)
            permille = 0xFFFF;

        _current = (uint16_t)permille;
        _ring[_index] = _current;
        _index = (uint8_t)((_index + 1) % HISTORY);
        if (_filled < HISTORY)
            _filled++;
        if (_current > _peak)
            _peak = _current;
#endif
    }

    void BusLoad::reset()
    {
        for (uint8_t i = 0; i < HISTORY; i++)
            _ring[i] = 0;
        _index = 0;
        _filled = 0;
        _current = 0;
        _peak = 0;
        _currentBytes = 0;
        _peakBytes = 0;
        _armed = false; // what was counted before now belongs to no measured second
    }

    uint16_t BusLoad::averagePermille() const
    {
        if (_filled == 0)
            return 0;

        uint32_t sum = 0;
        for (uint8_t i = 0; i < _filled; i++)
            sum += _ring[i];
        return (uint16_t)(sum / _filled);
    }

    uint16_t BusLoad::historyAt(uint8_t index) const
    {
        if (index >= _filled)
            return 0;
        // Oldest first: with a full ring the oldest slot is the one the next write will replace.
        const uint8_t start = (_filled == HISTORY) ? _index : 0;
        return _ring[(uint8_t)((start + index) % HISTORY)];
    }
} // namespace OpenKNX
