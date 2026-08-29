/**
 * @file        BusLoad.h
 * @brief       TP1 bus load: real line occupancy, sampled once per second
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#pragma once

#include <stdint.h>

namespace OpenKNX
{
    /**
     * @brief Share of TP1 line time occupied; 100% = the line cannot carry another telegram.
     *
     * Uses the TPUart frame bit counter (exact line time per frame) against the 9600 bit/s capacity,
     * NOT Statistics::getBusLoad() -- that getter resets the window it measures, so concurrent
     * readers corrupt each other.
     * TODO(tpuart-v2): book by frame timestamp; frames now count in the second they complete.
     */
    class BusLoad
    {
      public:
        static constexpr uint16_t TP1_BIT_PER_SEC = 9600;
        static constexpr uint8_t BIT_PER_OCTET = 13; // 11 data bits + 2 bits to the next character
        static constexpr uint8_t HISTORY = 60;       // one minute

        /** @brief 1 Hz sampler; a plain millis() compare on every other loop pass. */
        void loop();

        /** @brief Drop history, average and peak; measuring restarts with the next second. */
        void reset();

        /** @brief Occupancy in tenths of a percent (183 = 18.3%); 1000 = line full. */
        uint16_t currentPermille() const { return _current; }
        uint16_t peakPermille() const { return _peak; }
        uint16_t averagePermille() const;

        /** @brief Occupancy in whole percent, clamped to 100 -- for a DPT_Scaling group object. */
        uint8_t currentPercent() const { return _current >= 1000 ? 100 : (uint8_t)(_current / 10); }

        /** @brief Raw receive volume of the last second, for a "x B/s" readout. */
        uint16_t currentBytesPerSec() const { return _currentBytes; }
        uint16_t peakBytesPerSec() const { return _peakBytes; }

        /** @brief i-th history slot in permille, 0 = oldest; 0 for slots not filled yet. */
        uint16_t historyAt(uint8_t index) const;
        uint8_t historyCount() const { return _filled; }

      private:
        uint16_t _ring[HISTORY] = {};
        uint8_t _index = 0;
        uint8_t _filled = 0;
        uint16_t _current = 0;
        uint16_t _peak = 0;
        uint16_t _currentBytes = 0;
        uint16_t _peakBytes = 0;
        uint32_t _lastTick = 0;
        uint32_t _lastBytes = 0;
        uint32_t _lastBits = 0;
        uint32_t _lastDiscarded = 0;
        bool _armed = false;
    };
} // namespace OpenKNX
