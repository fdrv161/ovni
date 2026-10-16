#pragma once
#include <cmath>
#include <complex>

namespace telescope
{
// ========================================================================================================
// Weighting curves for the SPECTRUM lens (Periscope 1.1.0), after TDR Prism's "Analyzer Weighting".
//
// A weighting is a DISPLAY transformation, like the slope it replaces: W(f) in dB is added to the level of
// every bin (or band) at frequency f. Nothing in the engine changes. All curves are 0 dB at 1 kHz, so a
// 1 kHz tone reads the same under every curve. "Slope" is one choice of the same menu, as in Prism: with it
// the lens behaves exactly as before.
//
// Sources and check values: _research/spectrum-weighting-masking.md (A, B, C, D from IEC 61672 / 651 / 537
// via the public formulas; K from ITU-R BS.1770 with the libebur128 derivation for any sample rate; 468 from
// the closed-form fit of ITU-R BS.468-4; ISO 226:2003 at 80 phon, inverted; X-curve from public secondary
// sources, inverted).
// ========================================================================================================
namespace weighting
{
enum Curve { slope = 0, aWeight, bWeight, cWeight, dWeight, kWeight, itu468, iso226, xCurve, kNumCurves };

// Display names, the same in every language (they are standard names). Spoken names live in the lens.
inline const char* name (int curve) noexcept
{
    static const char* const names[kNumCurves] = { "Slope", "A", "B", "C", "D", "K", "ITU-R 468",
                                                   "ISO 226", "X-curve" };
    return curve >= 0 && curve < kNumCurves ? names[curve] : names[0];
}

namespace detail
{
inline double sq (double x) noexcept { return x * x; }

inline double aDb (double f) noexcept
{
    const double f2 = f * f;
    const double r = sq (12194.0) * f2 * f2
                   / ((f2 + sq (20.6)) * std::sqrt ((f2 + sq (107.7)) * (f2 + sq (737.9))) * (f2 + sq (12194.0)));
    return 20.0 * std::log10 (r) + 2.00;
}

inline double bDb (double f) noexcept
{
    const double f2 = f * f;
    const double r = sq (12194.0) * f2 * f
                   / ((f2 + sq (20.6)) * std::sqrt (f2 + sq (158.5)) * (f2 + sq (12194.0)));
    return 20.0 * std::log10 (r) + 0.17;
}

inline double cDb (double f) noexcept
{
    const double f2 = f * f;
    const double r = sq (12194.0) * f2 / ((f2 + sq (20.6)) * (f2 + sq (12194.0)));
    return 20.0 * std::log10 (r) + 0.06;
}

inline double dDb (double f) noexcept
{
    const double f2 = f * f;
    const double h = (sq (1037918.48 - f2) + 1080768.16 * f2) / (sq (9837328.0 - f2) + 11723776.0 * f2);
    const double r = f / 6.8966888496476e-5 * std::sqrt (h / ((f2 + 79919.29) * (f2 + 1345600.0)));
    return 20.0 * std::log10 (r);
}

// One biquad of the K filter, designed for `fs` the way libebur128 does it (bilinear transform from the
// analogue parameters; at 48 kHz it gives the BS.1770 coefficients to all printed digits).
inline std::complex<double> kStage (bool shelf, double f, double fs) noexcept
{
    const double pi = 3.14159265358979323846;
    double b0, b1, b2, a1, a2;
    if (shelf)
    {
        const double f0 = 1681.974450955533, g = 3.999843853973347, q = 0.7071752369554196;
        const double k = std::tan (pi * f0 / fs);
        const double vh = std::pow (10.0, g / 20.0), vb = std::pow (vh, 0.4996667741545416);
        const double a0 = 1.0 + k / q + k * k;
        b0 = (vh + vb * k / q + k * k) / a0;
        b1 = 2.0 * (k * k - vh) / a0;
        b2 = (vh - vb * k / q + k * k) / a0;
        a1 = 2.0 * (k * k - 1.0) / a0;
        a2 = (1.0 - k / q + k * k) / a0;
    }
    else
    {
        const double f0 = 38.13547087602444, q = 0.5003270373238773;
        const double k = std::tan (pi * f0 / fs);
        const double a0 = 1.0 + k / q + k * k;
        b0 = 1.0; b1 = -2.0; b2 = 1.0;
        a1 = 2.0 * (k * k - 1.0) / a0;
        a2 = (1.0 - k / q + k * k) / a0;
    }
    const std::complex<double> z1 = std::polar (1.0, -2.0 * pi * f / fs);   // z^-1 on the unit circle
    const std::complex<double> z2 = z1 * z1;
    return (b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2);
}

inline double kRawDb (double f, double fs) noexcept
{
    return 20.0 * std::log10 (std::abs (kStage (true, f, fs) * kStage (false, f, fs)));
}

inline double r468Db (double f) noexcept
{
    const double f2 = f * f, f3 = f2 * f, f4 = f2 * f2, f5 = f4 * f, f6 = f3 * f3;
    const double h1 = -4.737338981378384e-24 * f6 + 2.043828333606125e-15 * f4 - 1.363894795463638e-7 * f2 + 1.0;
    const double h2 =  1.306612257412824e-19 * f5 - 2.118150887518656e-11 * f3 + 5.559488023498642e-4 * f;
    const double r  = 1.246332637532143e-4 * f / std::sqrt (h1 * h1 + h2 * h2);
    return 18.2 + 20.0 * std::log10 (r);
}

// ISO 226:2003 Table 1: frequency, alpha_f, L_U, T_f.
struct IsoRow { double f, af, lu, tf; };
inline constexpr IsoRow kIso[] = {
    { 20, 0.532, -31.6, 78.5 }, { 25, 0.506, -27.2, 68.7 }, { 31.5, 0.480, -23.0, 59.5 }, { 40, 0.455, -19.1, 51.1 },
    { 50, 0.432, -15.9, 44.0 }, { 63, 0.409, -13.0, 37.5 }, { 80, 0.387, -10.3, 31.5 }, { 100, 0.367, -8.1, 26.5 },
    { 125, 0.349, -6.2, 22.1 }, { 160, 0.330, -4.5, 17.9 }, { 200, 0.315, -3.1, 14.4 }, { 250, 0.301, -2.0, 11.4 },
    { 315, 0.288, -1.1, 8.6 }, { 400, 0.276, -0.4, 6.2 }, { 500, 0.267, 0.0, 4.4 }, { 630, 0.259, 0.3, 3.0 },
    { 800, 0.253, 0.5, 2.2 }, { 1000, 0.250, 0.0, 2.4 }, { 1250, 0.246, -2.7, 3.5 }, { 1600, 0.244, -4.1, 1.7 },
    { 2000, 0.243, -1.0, -1.3 }, { 2500, 0.243, 1.7, -4.2 }, { 3150, 0.243, 2.5, -6.0 }, { 4000, 0.242, 1.2, -5.4 },
    { 5000, 0.242, -2.1, -1.5 }, { 6300, 0.245, -7.1, 6.0 }, { 8000, 0.254, -11.2, 12.6 }, { 10000, 0.271, -10.7, 13.9 },
    { 12500, 0.301, -3.1, 12.3 },
};
inline constexpr int kIsoRows = (int) (sizeof (kIso) / sizeof (kIso[0]));

// SPL of a pure tone that sounds as loud as `phon` at 1 kHz (ISO 226:2003 clause 4.1).
inline double isoLp (const IsoRow& r, double phon) noexcept
{
    const double af = 4.47e-3 * (std::pow (10.0, 0.025 * phon) - 1.15)
                    + std::pow (0.4 * std::pow (10.0, (r.tf + r.lu) / 10.0 - 9.0), r.af);
    return 10.0 / r.af * std::log10 (af) - r.lu + 94.0;
}

// The inverse contour at 80 phon: how much louder a component at f sounds than the same level at 1 kHz.
// Linear in log f between the table points; the end values are held below 20 Hz and above 12.5 kHz (the
// standard has no data there).
inline double iso226Db (double f) noexcept
{
    static const auto table = []
    {
        struct T { double logF[kIsoRows]; double w[kIsoRows]; } t {};
        const double lp1k = isoLp (kIso[17], 80.0);
        for (int i = 0; i < kIsoRows; ++i)
        {
            t.logF[i] = std::log10 (kIso[i].f);
            t.w[i]    = lp1k - isoLp (kIso[i], 80.0);
        }
        return t;
    }();
    const double lf = std::log10 (f);
    if (lf <= table.logF[0]) return table.w[0];
    if (lf >= table.logF[kIsoRows - 1]) return table.w[kIsoRows - 1];
    int i = 0;
    while (i < kIsoRows - 2 && lf > table.logF[i + 1]) ++i;
    const double t = (lf - table.logF[i]) / (table.logF[i + 1] - table.logF[i]);
    return table.w[i] + t * (table.w[i + 1] - table.w[i]);
}

// Inverse cinema X-curve: flat up to 2 kHz, then rising 3 dB per octave (the X-curve falls 3 dB per octave
// above 2 kHz). Sources disagree above 10 kHz; this keeps 3 dB per octave until a Prism screenshot says
// otherwise (see the research file).
inline double xDb (double f) noexcept
{
    return f <= 2000.0 ? 0.0 : 3.0 * std::log2 (f / 2000.0);
}
}

// W(f) in dB for `curve`. `sr` matters only for K (the digital filter of the session's sample rate);
// `slopeDbPerOct` only for Slope. Frequencies below 10 Hz are treated as 10 Hz: every curve but Slope is
// -inf or undefined at DC.
inline float db (int curve, double hz, double sr, float slopeDbPerOct) noexcept
{
    if (curve == slope)
        return (slopeDbPerOct == 0.0f || hz <= 0.0) ? 0.0f : slopeDbPerOct * (float) std::log2 (hz / 1000.0);

    const double f  = hz < 10.0 ? 10.0 : hz;
    const double fs = sr > 0.0 ? sr : 48000.0;
    switch (curve)
    {
        case aWeight: return (float) detail::aDb (f);
        case bWeight: return (float) detail::bDb (f);
        case cWeight: return (float) detail::cDb (f);
        case dWeight: return (float) detail::dDb (f);
        // K and 468: normalised so that 1 kHz is exactly 0 dB (raw K is +0.7 dB there, raw 468 -0.04 dB).
        case kWeight: return f >= fs * 0.5 ? (float) (detail::kRawDb (fs * 0.499, fs) - detail::kRawDb (1000.0, fs))
                                           : (float) (detail::kRawDb (f, fs) - detail::kRawDb (1000.0, fs));
        case itu468:  return (float) (detail::r468Db (f) - detail::r468Db (1000.0));
        case iso226:  return (float) detail::iso226Db (f);
        case xCurve:  return (float) detail::xDb (f);
        default:      return 0.0f;
    }
}
}
}
