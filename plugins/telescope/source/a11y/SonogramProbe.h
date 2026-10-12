#pragma once
#include "a11y/A11yWidgets.h"
#include "a11y/SpokenFormat.h"
#include "analysis/SpectrogramRing.h"
#include <algorithm>
#include <cmath>

// ========================================================================================================
// SonogramProbe — the keyboard probe of SPECTROGRAM, STEREO SPECTROGRAM and WATERFALL.
//
// On screen the mouse reads one point of the picture: how long ago, the frequency and the level (and the
// coherence in the stereo one). The probe reads the same ring at a moment and a frequency set from the
// keyboard, so a screen reader user gets the same numbers.
//
// One probe position is a cell: a quarter tone either side of its note and half a time step either side of
// its moment. The cell answers with its loudest point, so a click or a narrow tone that falls between two
// probe steps is not missed (the eye sees it there even when the mouse is not exactly on it). The level bytes
// are read straight from the ring, like the mouse readout, never from the colour.
// ========================================================================================================
namespace telescope::sonoprobe
{
inline constexpr int    kMinMidi  = 16;     // 20.6 Hz, the bottom of the plot
inline constexpr int    kMaxMidi  = 135;    // 19.9 kHz, the top
inline constexpr double kTimeStep = 0.25;   // seconds per probe step in time

inline double midiHz (int midi) { return 440.0 * std::pow (2.0, (double) (midi - 69) / 12.0); }

// The coarse step in time: one mark of the time axis that the screen draws for this span.
inline double tickSeconds (double span) { return span <= 12.0 ? 2.0 : (span <= 34.0 ? 5.0 : 10.0); }

struct Probe
{
    int midi = 69;        // the probe sits on notes, like the SPECTRUM one: 69 = A 4, 440 Hz
    int timeSteps = 0;    // how far back, in kTimeStep: 0 = now

    double hz() const { return midiHz (midi); }

    // The moment the probe looks at. Clamped to what the screen shows now: the window may have shrunk since.
    double seconds (double span) const
    {
        return std::min ((double) timeSteps * kTimeStep, std::max (0.0, span));
    }

    // Plus and minus move a semitone, the coarse step an octave, the extreme step to an edge of the plot.
    bool stepFrequency (int step)
    {
        const int size = std::abs (step) >= a11yui::kExtreme ? 1000 : (std::abs (step) >= a11yui::kCoarse ? 12 : 1);
        const int next = std::clamp (midi + (step > 0 ? size : -size), kMinMidi, kMaxMidi);
        if (next == midi) return false;
        midi = next;
        return true;
    }

    // Plus goes further back, minus towards now. The coarse step is one mark of the time axis, the extreme
    // step now (Home) or the oldest moment on screen (End).
    bool stepTime (int step, double span)
    {
        const int last   = std::max (0, (int) std::floor (span / kTimeStep + 1.0e-9));
        const int coarse = std::max (1, (int) std::lround (tickSeconds (span) / kTimeStep));
        const int size   = std::abs (step) >= a11yui::kExtreme ? last + 1 : (std::abs (step) >= a11yui::kCoarse ? coarse : 1);
        const int from   = std::min (timeSteps, last);
        const int next   = std::clamp (from + (step > 0 ? size : -size), 0, last);
        if (next == timeSteps) return false;
        timeSteps = next;
        return true;
    }
};

struct Cell
{
    bool valid = false;      // false: the ring holds nothing at that moment (not written yet, or just cleared)
    int  level = 0;          // the level byte at the probe, 0..255 over [-range, 0]
    int  coherence = 128;    // stereo ring only: the coherence byte of that point (0 = -1, 128 = 0, 255 = +1)
    int  loudestRow = -1;    // the loudest row of the whole moment, over all frequencies (-1: silence)
    int  loudestLevel = 0;
};

// The cell at `secondsAgo` (0 = the newest column) around `hz`. `halfSeconds` 0 reads the newest column alone.
template <int B>
Cell cellAt (const SpectrogramRingT<B>& ring, double secondsAgo, double halfSeconds, double hz)
{
    using Ring = SpectrogramRingT<B>;
    Cell c;
    const double    cps = ring.columnsPerSecond();
    const long long w   = ring.writeIndex();
    const long long n   = (long long) ring.count();
    if (n <= 0 || cps <= 0.0) return c;

    const long long newest = w - 1;
    const long long from = std::max (w - n, newest - (long long) std::llround ((secondsAgo + halfSeconds) * cps));
    const long long to   = std::min (newest, newest - (long long) std::llround (std::max (0.0, secondsAgo - halfSeconds) * cps));

    // Rows are log spaced from 20 Hz to 20 kHz, about two rows per quarter tone.
    const double octaves = std::log2 (Ring::kMaxHz / Ring::kMinHz);
    const double centre  = (double) (Ring::kRows - 1) * std::log2 (std::clamp (hz, Ring::kMinHz, Ring::kMaxHz) / Ring::kMinHz) / octaves;
    const double quarter = (double) (Ring::kRows - 1) / (24.0 * octaves);
    const int lo = std::clamp ((int) std::ceil (centre - quarter), 0, Ring::kRows - 1);
    const int hi = std::clamp ((int) std::floor (centre + quarter), lo, Ring::kRows - 1);

    juce::uint8 col[Ring::kCellBytes];
    int best = -1;   // packed (level << 8) | coherence: the louder wins and a tie goes to the higher coherence, as painted
    for (long long i = from; i <= to; ++i)
    {
        if (! ring.copyColumn (i, col)) continue;
        c.valid = true;
        for (int r = 0; r < Ring::kRows; ++r)
        {
            // One byte per cell: the level. Two bytes: coherence first, then the level.
            const int level = (int) col[B * r + (B - 1)];
            if (level > c.loudestLevel) { c.loudestLevel = level; c.loudestRow = r; }
            if (r >= lo && r <= hi)
                best = std::max (best, (level << 8) | (B == 2 ? (int) col[B * r] : 128));
        }
    }
    if (best >= 0) { c.level = best >> 8; c.coherence = best & 0xff; }
    return c;
}

inline float levelDb (int level, int rangeDb)
{
    return (float) ((double) level / 255.0 * (double) rangeDb - (double) rangeDb);
}

// Byte 0 is the floor of the range: anything quieter is drawn the same, so it is spoken as below the range.
inline juce::String levelText (int level, int rangeDb)
{
    return spoken::db (level <= 0 ? -1.0e9f : levelDb (level, rangeDb), rangeDb);
}

inline juce::String timeText (double secondsAgo)
{
    if (secondsAgo <= 0.0) return a11ytext::T ("now");
    return a11ytext::TF ("{0} seconds ago", { spoken::trimmed (secondsAgo, 2) });
}

// The brightest point of that moment, as the eye finds it in the column.
template <int B>
juce::String loudestText (const Cell& c, int rangeDb)
{
    if (c.loudestRow < 0) return a11ytext::T ("silence");
    return a11ytext::TF ("loudest at that moment: {0}, {1}",
                         { spoken::hz (SpectrogramRingT<B>::rowFrequency (c.loudestRow)), levelText (c.loudestLevel, rangeDb) });
}
}
