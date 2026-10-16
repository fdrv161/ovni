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
// The model: ISO/IEC 11172-3 Psychoacoustic Model 2, the base of the informative AAC model, on 1/3-Bark
// partitions, with tonality from the spectral flatness of each partition's neighbourhood (Johnston 1988)
// instead of the phase-prediction measure, which needs frames a few milliseconds apart (ours are not). Every
// constant and the worked example are in _research/spectrum-weighting-masking.md, section 2; a Python copy of
// this exact algorithm reproduces that example (masked 700-960 Hz and 1-1.9 kHz around a 1 kHz tone at
// -10 dBFS over pink noise at -40 dBFS).
//
// Input: the power of each bin as the SPECTRUM lens measures it (a full-scale sine reads 1.0 in its peak bin,
// see SpectrumFrame.h), for the bins below 20 kHz. Output: one state per partition. Tables are built in
// configure() off the audio thread; compute() allocates nothing.
// ========================================================================================================
class AuditoryMasking
{
public:
    enum State : std::uint8_t { audible = 0, masked = 1, belowHearing = 2 };

    // Tone-masking-noise and noise-masking-tone offsets in dB: the AAC values (TMN 18, NMT 6), as quoted from
    // the AAC model. Model 2 itself uses about 24.5 and 5.5; the Prism screenshots decide (see the research).
    static constexpr double kTmnDb = 18.0, kNmtDb = 6.0;
    // The threshold in quiet is in dB SPL; a full-scale sine is taken as 96 dB SPL (the usual MPEG choice).
    static constexpr double kFullScaleSpl = 96.0;

    struct Partition { int binLo = 0, binHi = 0, sfmLo = 0, sfmHi = 0; double zc = 0.0, athPower = 0.0; float loHz = 0.0f, hiHz = 0.0f; };

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
            // power: a sine sums to A²·ENBW there and to A²/2 as energy, and a full-scale sine is 96 dB SPL.
            p.athPower = std::pow (10.0, (athMin - kFullScaleSpl) / 10.0) * enbw;

            // Spectral flatness is measured over the bins within half a Bark of the centre.
            p.sfmLo = p.binLo;
            while (p.sfmLo > 1 && bark ((double) (p.sfmLo - 1) * binHz) >= p.zc - 0.5) --p.sfmLo;
            p.sfmHi = p.binHi;
            while (p.sfmHi < bins - 1 && (double) (p.sfmHi + 1) * binHz <= 20000.0
                   && bark ((double) (p.sfmHi + 1) * binHz) <= p.zc + 0.5) ++p.sfmHi;
        }

        const size_t n = parts.size();
        spread.assign (n * n, 0.0);
        rnorm.assign (n, 0.0);
        for (size_t b = 0; b < n; ++b)
            for (size_t j = 0; j < n; ++j)
            {
                const double s = spreading (parts[b].zc - parts[j].zc);
                spread[b * n + j] = s;
                rnorm[b] += s;
            }
        energy.assign (n, 0.0);
        tonality.assign (n, 0.0);
        states.assign (n, audible);
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
            const auto& p = parts[b];
            double e = 0.0;
            for (int k = p.binLo; k <= p.binHi && k < numBins; ++k) e += power[k];
            energy[b] = e;

            // Tonality from spectral flatness: 1 for a pure tone, 0 for noise (SFM of -60 dB or flatter).
            double sum = 0.0, logSum = 0.0;
            int    count = 0;
            for (int k = p.sfmLo; k <= p.sfmHi && k < numBins; ++k)
            {
                sum    += power[k];
                logSum += std::log (power[k] + 1.0e-30);
                ++count;
            }
            if (count == 0 || sum <= 0.0) { tonality[b] = 0.0; continue; }
            const double gm  = std::exp (logSum / (double) count);
            const double sfm = 10.0 * std::log10 (std::max (gm, 1.0e-30) / (sum / (double) count));
            tonality[b] = std::clamp (sfm / -60.0, 0.0, 1.0);
        }

        for (size_t b = 0; b < n; ++b)
        {
            // Spread energy and spread (energy-weighted) tonality from every partition onto this one.
            double ecb = 0.0, tonal = 0.0;
            const double* row = spread.data() + b * n;
            for (size_t j = 0; j < n; ++j)
            {
                const double c = energy[j] * row[j];
                ecb   += c;
                tonal += c * tonality[j];
            }
            if (ecb <= 0.0) { states[b] = belowHearing; continue; }
            const double tb  = tonal / ecb;
            const double snr = tb * kTmnDb + (1.0 - tb) * kNmtDb;
            const double threshold = ecb / rnorm[b] * std::pow (10.0, -snr / 10.0);

            // A partition's own energy never masks it: s(0) = 1, rnorm >= 1 and snr >= 6 dB.
            if (energy[b] < threshold)           states[b] = masked;
            else if (energy[b] < parts[b].athPower) states[b] = belowHearing;
            else                                 states[b] = audible;
        }
    }

    // Zwicker and Terhardt (1980).
    static double bark (double f) noexcept
    {
        return 13.0 * std::atan (0.00076 * f) + 3.5 * std::atan ((f / 7500.0) * (f / 7500.0));
    }

private:
    // Model 2 spreading function (ISO 11172-3, as in toolame psycho_2.c); dz = maskee - masker in Bark.
    // Masking reaches much further up in frequency than down.
    static double spreading (double dz) noexcept
    {
        const double t = 1.05 * dz;
        const double x = (t >= 0.5 && t <= 2.5) ? 8.0 * ((t - 0.5) * (t - 0.5) - 2.0 * (t - 0.5)) : 0.0;
        const double y = 15.811389 + 7.5 * (t + 0.474) - 17.5 * std::sqrt (1.0 + (t + 0.474) * (t + 0.474));
        return y <= -100.0 ? 0.0 : std::pow (10.0, (x + y) / 10.0);
    }

    // Threshold in quiet, dB SPL (Terhardt 1979).
    static double athSpl (double f) noexcept
    {
        const double k = f / 1000.0;
        return 3.64 * std::pow (k, -0.8) - 6.5 * std::exp (-0.6 * (k - 3.3) * (k - 3.3)) + 1.0e-3 * k * k * k * k;
    }

    std::vector<Partition>    parts;
    std::vector<double>       spread, rnorm, energy, tonality;
    std::vector<std::uint8_t> states;
};
}
