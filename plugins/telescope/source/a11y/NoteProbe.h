#pragma once
#include "a11y/A11yWidgets.h"
#include "a11y/SpokenFormat.h"
#include "lenses/NoteName.h"
#include <algorithm>
#include <cmath>
#include <vector>

// ========================================================================================================
// NoteProbe — the keyboard probe of CQT and SPIRAL.
//
// On screen the mouse reads one bar (CQT) or one spike (SPIRAL): the note, the frequency and the level of
// that constant-Q bin. The probe walks the same bins by notes. The grid has two bins per semitone: the
// note's own bin and, between two notes, a quarter-tone bin that the screen draws fainter. The probe answers
// with the louder of the note's bin and the quarter-tone bins either side (the note's own bin on a tie), and
// a quarter-tone answer is spoken with its cents, so a detuned note between two keys is not missed.
// ========================================================================================================
namespace telescope::noteprobe
{
inline double midiHz (int midi) { return 440.0 * std::pow (2.0, (double) (midi - 69) / 12.0); }

// The bin of a note in a constant-Q grid that starts at fMin with bpo bins per octave (not rounded).
inline double binOf (int midi, float fMin, int bpo)
{
    return (double) bpo * std::log2 (midiHz (midi) / (double) fMin);
}

// The notes the grid holds: from the first note at or above fMin to the last at or below the top bin.
inline int lowestMidi (float fMin)
{
    return (int) std::ceil (69.0 + 12.0 * std::log2 ((double) fMin / 440.0) - 1.0e-6);
}
inline int highestMidi (float fMin, int bins, int bpo)
{
    if (bins <= 0 || bpo <= 0) return lowestMidi (fMin);
    const double top = (double) fMin * std::pow (2.0, (double) (bins - 1) / (double) bpo);
    return std::max (lowestMidi (fMin), (int) std::floor (69.0 + 12.0 * std::log2 (top / 440.0) + 1.0e-6));
}

// Plus and minus move a semitone, the coarse step an octave, the extreme step to an end of the grid.
inline bool step (int& midi, int stepValue, float fMin, int bins, int bpo)
{
    const int size = std::abs (stepValue) >= a11yui::kExtreme ? 1000 : (std::abs (stepValue) >= a11yui::kCoarse ? 12 : 1);
    const int next = std::clamp (midi + (stepValue > 0 ? size : -size), lowestMidi (fMin), highestMidi (fMin, bins, bpo));
    if (next == midi) return false;
    midi = next;
    return true;
}

struct Reading
{
    bool   valid = false;
    int    bin = -1;
    int    cents = 0;      // where the answering bin sits against the probe's note: 0, or +-50 for a quarter tone
    double hz = 0.0;
    float  db = 0.0f;
};

inline Reading read (const std::vector<float>& db, int bins, float fMin, int bpo, int midi)
{
    Reading r;
    const int n = std::min (bins, (int) db.size());
    if (n <= 0 || bpo <= 0) return r;
    const double exact = binOf (midi, fMin, bpo);
    const int k = (int) std::lround (exact);
    if (k < 0 || k >= n) return r;

    int best = k;
    if (bpo > 12)
        for (const int j : { k - 1, k + 1 })
            if (j >= 0 && j < n && db[(size_t) j] > db[(size_t) best]) best = j;

    r.valid = true;
    r.bin   = best;
    r.hz    = (double) fMin * std::pow (2.0, (double) best / (double) bpo);
    r.cents = (int) std::lround (1200.0 * ((double) best - exact) / (double) bpo);
    r.db    = db[(size_t) best];
    return r;
}

// "A 4 plus 50 cents": the probe's note, with the cents of the bin that answered.
inline juce::String noteText (const Reading& r, int midi)
{
    NoteReadout n;
    n.midi  = midi;
    n.cents = r.cents;
    return spoken::noteWithCents (n);
}
}
