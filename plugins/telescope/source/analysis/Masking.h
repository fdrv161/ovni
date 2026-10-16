#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace telescope
{
// ========================================================================================================
// AuditoryMasking — which parts of the spectrum are masked by louder sounds nearby (Periscope 1.1.0), after
// TDR Prism's "Auditory Masking", which follows the AAC psychoacoustic model.
//
// The model is the AAC encoder model of 3GPP TS 26.403 (the one FFmpeg's aacpsy.c implements): in each
// 1/3-Bark partition the masking threshold is the partition's energy 29 dB down, spread to the neighbours
// with a fixed slope per Bark (further up in frequency than down), and never below the threshold in quiet.
// A partition is masked when its own energy is under the threshold its neighbours spread onto it.
//
// It was chosen over ISO 11172-3 Model 2 by comparing both with Prism (Screens/prism, 2026-10-16): around a
// 1 kHz tone at -10 dBFS over pink noise at -40 dBFS, Prism masks about 850 Hz to 1.32 kHz and marks
// everything above about 17.5 kHz as below hearing. Model 2 masked 700 Hz to 1.9 kHz. This model, with the
// upward slope at 12 dB per Bark (3GPP says 15) and a full-scale sine taken as 100 dB SPL, gives 873 Hz to
// 1.32 kHz and 17.7 kHz. Research and prototypes: _research/spectrum-weighting-masking.md,
// _diag/tools/masking_3gpp.py.
//
// Input: the power of each bin as the SPECTRUM lens measures it (a full-scale sine reads 1.0 in its peak bin,
// see SpectrumFrame.h). Output: one state per partition below 20 kHz. Tables are built in configure() off the
// audio thread; compute() allocates nothing.
// ========================================================================================================
class AuditoryMasking
{
public:
    enum State : std::uint8_t { audible = 0, masked = 1, belowHearing = 2 };

    static constexpr double kSelfMaskRatio     = 0.001258925;   // -29 dB: a partition's own masking threshold
    static constexpr double kSpreadUpDbPerBark   = 12.0;        // towards higher frequencies
    static constexpr double kSpreadDownDbPerBark = 30.0;        // towards lower frequencies
    static constexpr double kFullScaleSpl      = 100.0;         // dB SPL of a full-scale sine, for the threshold in quiet

    struct Partition { int binLo = 0, binHi = 0; double zc = 0.0, athPower = 0.0, upFactor = 0.0, downFactor = 0.0;
                       float loHz = 0.0f, hiHz = 0.0f; };

    // `enbw`: equivalent noise bandwidth of the window in bins (N·Σw²/(Σw)², 1.5 for Hann). It turns the
    // peak-scaled bin power into energy, which the threshold in quiet needs.
    void configure (double sr, int fftSize, double enbw)
    {
        parts.clear();
        const int    bins  = fftSize / 2 + 1;
        const double binHz = sr / (double) fftSize;

        // 1/3-Bark partitions: consecutive bins with the same floor (3 z). Bin 0 (DC) and anything above
        // 20 kHz are left out.
        int lastKey = -1;
        for (int k = 1; k < bins; ++k)
        {
            const double f = (double) k * binHz;
            if (f > 20000.0) break;
            const int key = (int) std::floor (3.0 * bark (f));
            if (key != lastKey) { parts.push_back ({}); parts.back().binLo = k; lastKey = key; }
            parts.back().binHi = k;
        }

        for (auto& p : parts)
        {
            double zsum = 0.0, athMin = 1.0e9;
            for (int k = p.binLo; k <= p.binHi; ++k)
            {
                const double f = (double) k * binHz;
                zsum  += bark (f);
                athMin = std::min (athMin, athSpl (std::max (f, 20.0)));
            }
            p.zc   = zsum / (double) (p.binHi - p.binLo + 1);
            p.loHz = (float) (((double) p.binLo - 0.5) * binHz);
            p.hiHz = (float) (((double) p.binHi + 0.5) * binHz);
            // Energy of the quietest audible sound in this partition, in the units of the summed peak-scaled
            // power: a sine sums to A²·ENBW there and to A²/2 as energy.
            p.athPower = std::pow (10.0, (athMin - kFullScaleSpl) / 10.0) * enbw;
        }

        // What each partition passes on to its neighbour, by the Bark distance between their centres.
        for (size_t b = 0; b < parts.size(); ++b)
        {
            if (b > 0)
                parts[b].upFactor = std::pow (10.0, -kSpreadUpDbPerBark / 10.0 * (parts[b].zc - parts[b - 1].zc));
            if (b + 1 < parts.size())
                parts[b].downFactor = std::pow (10.0, -kSpreadDownDbPerBark / 10.0 * (parts[b + 1].zc - parts[b].zc));
        }
        energy.assign (parts.size(), 0.0);
        threshold.assign (parts.size(), 0.0);
        states.assign (parts.size(), audible);
    }

    int numPartitions() const noexcept { return (int) parts.size(); }
    const Partition& partition (int i) const noexcept { return parts[(size_t) i]; }
    const std::vector<std::uint8_t>& result() const noexcept { return states; }

    void compute (const double* power, int numBins)
    {
        const size_t n = parts.size();
        if (n == 0) return;

        for (size_t b = 0; b < n; ++b)
        {
            double e = 0.0;
            for (int k = parts[b].binLo; k <= parts[b].binHi && k < numBins; ++k) e += power[k];
            energy[b]    = e;
            threshold[b] = e * kSelfMaskRatio;
        }
        // Spread up in frequency (low to high), then down (high to low), keeping the larger threshold.
        for (size_t b = 1; b < n; ++b)
            threshold[b] = std::max (threshold[b], threshold[b - 1] * parts[b].upFactor);
        for (size_t b = n - 1; b-- > 0;)
            threshold[b] = std::max (threshold[b], threshold[b + 1] * parts[b].downFactor);

        // The partition's own share of the threshold is 29 dB under its energy, so only neighbours can mask it.
        for (size_t b = 0; b < n; ++b)
        {
            if (energy[b] < threshold[b])              states[b] = masked;
            else if (energy[b] < parts[b].athPower)    states[b] = belowHearing;
            else                                       states[b] = audible;
        }
    }

    // Zwicker and Terhardt (1980).
    static double bark (double f) noexcept
    {
        return 13.0 * std::atan (0.00076 * f) + 3.5 * std::atan ((f / 7500.0) * (f / 7500.0));
    }

private:
    // Threshold in quiet, dB SPL: Terhardt's formula as FFmpeg's AAC model uses it (with its 8.7 kHz term and
    // ATH_ADD = 4), which matches the top of Prism's below-hearing region better than the plain formula.
    static double athSpl (double f) noexcept
    {
        const double k = f / 1000.0;
        return 3.64 * std::pow (k, -0.8) - 6.8 * std::exp (-0.6 * (k - 3.4) * (k - 3.4))
             + 6.0 * std::exp (-0.15 * (k - 8.7) * (k - 8.7)) + (0.6 + 0.04 * 4.0) * 0.001 * k * k * k * k;
    }

    std::vector<Partition>    parts;
    std::vector<double>       energy, threshold;
    std::vector<std::uint8_t> states;
};
}
