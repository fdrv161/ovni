#pragma once
#include <vector>
#include "analysis/SpectrumFrame.h"
#include "analysis/Weighting.h"
#include "analysis/modules/Spectrum.h"
#include "lenses/Lens.h"
#include "lenses/Raster.h"
#include "lenses/NoteName.h"

namespace telescope
{
class TelescopeProcessor;

// ========================================================================================================
// SPECTRUM — la lente 3, y la que más gente va a abrir. Un analizador de espectro completo:
//
//   arriba      el espectro sobre eje de frecuencia LOG (20 Hz - 20 kHz) con rejilla de décadas y ⅓ de
//               octava, y eje de dB con 0 arriba y el rango elegido (60 / 90 / 120 dB) hacia abajo.
//               Área rellena + línea; en L+R, dos espectros con los dos colores del tema. El peak hold va
//               como línea fina encima. En modo ⅓ de octava o Bark, barras en vez de curva.
//   al pasar    línea vertical + lectura: frecuencia, NOTA con sus cents (A4 = 440) y el dB del bin o de
//               la banda que está debajo del cursor. La nota es lo que convierte "hay algo en 82 Hz" en
//               "es un mi grave" sin salir del plugin.
//   abajo       la tira de controles (patrón de SCOPE, cada botón CICLA su opción): tamaño de FFT,
//               ventana, solape, canal, modo de bandas · slope, promediado, hold, rango.
//
// EL DIBUJO ES POR COLUMNA DE PÍXEL, CON MAX-HOLD. Con orden 15 hay 16 385 bins para ~1 000 píxeles: una
// muestra por píxel se comería 15 de cada 16 picos, y justo los picos son lo que uno mira. Cada columna
// toma el MÁXIMO de los bins que le tocan (y donde sobran píxeles por bin, interpola en dB). Nunca se
// generan más vértices que píxeles — es la misma lección que el osciloscopio de SCOPE.
//
// REDUCED MOTION: sin suavizado entre frames. La curva se dibuja tal cual llega, un cuadro coherente.
// ========================================================================================================
class SpectrumLens : public Lens
{
public:
    // ===== 57c · EL RECTÁNGULO DE LA CACHÉ, para VISUAL[hd] =====
    //
    // La mutación de la auditora sobre el 57b encontró que el test medía el gradiente del PANEL ENTERO, y
    // ahí la rejilla, los textos y los trazos vectoriales —que se dibujan a escala física siempre— tapan
    // lo que hace la caché: con la caché forzada de vuelta a 1×, SPECTRUM seguía dando 3.17. Midiendo
    // sólo adentro de este rectángulo, la razón habla de la caché y de nada más.
    juce::Rectangle<int> cacheAreaForTest() const noexcept { return zones.plot; }
    // 57c — la escala física con la que se horneó la caché. VISUAL[hd] lo verifica además de medir la
    // nitidez: es la comprobación ESTRUCTURAL de la regla de lenses/Raster.h, y la que hace imposible
    // que una mutación de esa clase pase inadvertida.
    float cacheScaleForTest() const noexcept { return curveCache.scale(); }

    explicit SpectrumLens (TelescopeProcessor& p);

    juce::String name() const override            { return kLensNames[(int) LensId::spectrum]; }
    LensId       id() const override              { return LensId::spectrum; }
    juce::uint32 requiredModules() const override { return kSpectrum; }
    // El idioma sale del estado del procesador (ver Lens::tr()).
    const juce::ValueTree& stateTree() const override;

    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    // ---- lectura de nota ----
    // La DEFINICIÓN vive en lenses/NoteName.h desde el prompt 52 (la piden también CQT y SPIRAL). Acá
    // queda el nombre con el que la llaman el test del 50 y la propia lente: mover la fórmula no puede
    // significar renombrar lo que ya estaba probado.
    using NoteReadout = telescope::NoteReadout;
    static NoteReadout noteFor (double freqHz) { return noteForFrequency (freqHz); }

    // Lo que devuelve la lectura bajo el cursor en la coordenada x (en píxeles del componente). Pública
    // para que el test pueda pedirla sin fabricar eventos de mouse.
    struct Readout { bool valid = false; double freqHz = 0.0; float db = 0.0f; NoteReadout note; };
    Readout readoutAtX (int x) const;

    // Los nueve controles, en el orden en que se dibujan. Público para que el test los cicle sin clicks.
    // Periscope 1.1.0: weightCurve is appended, so the author's indices keep their meaning.
    enum Control { fftSize = 0, window, overlap, channel, bandsMode, slope, average, hold, range,
                   smooth, weightCurve, maskCtl, kNumControls };
    void cycleControl (int control);

    // ================== ACCESSIBILITY (see Lens.h) ==================
    // Groups: readings (probe, signal tilt, shape, peaks and dips), third-octave bands, settings.
    int          a11yGroupCount() const override { return 3; }
    juce::String a11yGroupName (int group) const override;
    int          a11yRowCount (int group) const override;
    juce::String a11yRowName (int group, int row) const override;
    juce::String a11yRowValue (int group, int row) const override;
    bool         a11yIsAdjustable (int group, int row) const override;
    bool         a11yAdjust (int group, int row, int step) override;
    bool         a11yIsActivatable (int group, int row) const override;
    juce::String a11yActivate (int group, int row) override;
    juce::String a11yRowHelp (int group, int row) const override;
    bool         a11yTakeCursorRequest (int& group, int& row) override;

    // Setting from the keyboard: step forward/back without wrapping (the mouse still wraps).
    bool stepControl (int control, int step);

protected:
    void renderStatic (juce::Graphics&, int width, int height) override;
    void paintLive (juce::Graphics&) override;
    bool advanceFrame() override;

private:
    static constexpr double kMinHz = 20.0, kMaxHz = 20000.0;
    static constexpr float  kRelease = 0.35f;   // la curva SUBE de una y baja suave (es un medidor)
    static constexpr int    kScaleW  = 38;      // ancho del canal de etiquetas de dB
    static constexpr int    kAxisH   = 15;      // alto de la tira de frecuencias

    struct Zones
    {
        juce::Rectangle<int> plot, dbScale, freqAxis, footer;
        juce::Rectangle<int> button[kNumControls];
    };
    Zones zonesFor (int w, int h) const;

    double freqAtX (int x) const;               // x en coords del componente
    float  xForFreq (double hz) const;

    void rebuildColumns (const SpectrumFrame& f);
    // 57b — EL RASTERIZADOR PROPIO. Arma las curvas a resolución de DISPOSITIVO sobre la caché y la
    // devuelve al plano lógico de un solo blit. Ver el bloque grande de SpectrumLens.cpp.
    void rasteriseCurves (float physScale);
    void buildDeviceCurve (int slot, int deviceW, float physScale, int smoothRadius);
    void paintCurve (juce::Graphics&, int slot, juce::Colour) const;
    void paintBars (juce::Graphics&, int slot, juce::Colour) const;
    void paintReadout (juce::Graphics&) const;
    void paintButton (juce::Graphics&, juce::Rectangle<int>, const juce::String& label,
                      const juce::String& value, bool hovered) const;
    float yForDb (float db) const;

    // Periscope 1.1.0: the display weighting (see analysis/Weighting.h). weightAt is W(f) of the chosen curve,
    // Slope included; bandWeight is what the bars get: nothing with Slope (bands are flat for pink noise by
    // construction, see paintBars), W at the band centre with a weighting curve.
    float weightAt (double hz) const;
    float bandWeight (double centreHz) const;
    std::vector<float> colWeight;   // W at the centre of each plot column, rebuilt when its key changes
    int    colWeightCurve = -1, colWeightWidth = -1;
    float  colWeightSlope = -1.0f;
    double colWeightSr = 0.0;

    // Periscope 1.1.0: the masking of the latest frame (see SpectrumFrame::maskParts), for the overlay and speech.
    int         maskParts = 0;
    float       maskLoHz [SpectrumFrame::kMaxMaskParts] {};
    float       maskHiHz [SpectrumFrame::kMaxMaskParts] {};
    juce::uint8 maskState[SpectrumFrame::kMaxMaskParts] {};
    void paintMasking (juce::Graphics&) const;

    TelescopeProcessor& processor;

    // Los settings CACHEADOS. Viven en un ValueTree (11 propiedades con búsqueda por nombre): pedirlos
    // dentro de yForDb() —que corre una vez por columna de píxel— costaba 10 ms de pintado. Se refrescan
    // una vez por frame y una vez por horneado, que es cuando pueden cambiar.
    Spectrum::Settings sets;

    // Estado del último frame leído (no se copia el SpectrumFrame entero: son 260 KB).
    juce::uint32 lastFrameIndex = 0xffffffffu;
    int          fftSizeSeen = 0, numBinsSeen = 0, channelSeen = 0;
    int          lastSmoothIdx = -1;   // ver advanceFrame: los ajustes que cambian el dibujo
    bool visiblyMoved (float now, float before) const noexcept;   // el piso de silencio (ver el .cpp)
    std::vector<float> holdBefore[SpectrumFrame::kMaxSpectra];   // el hold del cuadro anterior (¿se movió?)
    double       srSeen = 48000.0;

    // Una entrada por COLUMNA DE PÍXEL del plot (ver el encabezado).
    std::vector<float> colDb[SpectrumFrame::kMaxSpectra];
    std::vector<float> colHold[SpectrumFrame::kMaxSpectra];
    std::vector<float> dispDb[SpectrumFrame::kMaxSpectra];   // la curva suavizada que se dibuja

    // ===== 57b: el camino de DIBUJO, en píxeles de dispositivo =====
    // `dev*` son las columnas de DISPOSITIVO (interpoladas desde las lógicas, ver buildDeviceCurve): son
    // lo que se DIBUJA. Los números del readout y los tests de bin siguen leyendo `colDb`, que es el DATO.
    raster::Cache      curveCache;
    std::vector<float> devY[SpectrumFrame::kMaxSpectra];      // y de la curva, en px de dispositivo
    std::vector<float> devHoldY[SpectrumFrame::kMaxSpectra];  // ídem del peak hold
    std::vector<float> devScratch, devSlope;                  // buffers del interpolador y del suavizado
    float bandDb  [SpectrumFrame::kMaxSpectra][SpectrumFrame::kNumThird] {};
    float bandHold[SpectrumFrame::kMaxSpectra][SpectrumFrame::kNumThird] {};
    float barkDb  [SpectrumFrame::kMaxSpectra][SpectrumFrame::kNumBark] {};

    Zones zones {};
    int   hovered = -1;      // botón bajo el cursor
    int   cursorX = -1;      // columna bajo el cursor dentro del plot, -1 = afuera
    int   lastRangeDb = 0;   // para rehornear la rejilla cuando cambia la escala

    // ---- accessibility: raw bins of the latest frame (the picture keeps only per-pixel columns) ----
    std::vector<float> a11yMag[SpectrumFrame::kMaxSpectra], a11yHold[SpectrumFrame::kMaxSpectra];
    int    a11yBins = 0, a11ySpectra = 0;
    double a11yBinHz = 0.0;
    bool   a11yHasFrame = false;
    juce::uint32 a11yFrameSerial = 0;          // grows with every new frame (curve cache)
    int    probeMidi = 69;                     // the probe sits on notes: 69 = A 4, 440 Hz
    bool   cursorToProbe = false;              // Enter on a peak asked the readings cursor to go to the probe

    // The curve for speech: the bins on a fixed log grid (96 points per octave, about one pixel column of a
    // big window), so the numbers do not depend on the window size. Stored without the slope; what is spoken
    // gets the slope added, as the plot draws it. See "the line" in SpectrumLens.cpp.
    struct A11yCurve
    {
        std::vector<float> rawDb[SpectrumFrame::kMaxSpectra];      // unsmoothed, per channel
        std::vector<float> holdDb[SpectrumFrame::kMaxSpectra];     // peak hold, never smoothed (as on screen)
        std::vector<float> smoothDb[SpectrumFrame::kMaxSpectra];   // smoothed like the drawn curve
        std::vector<float> top;                                    // louder channel, smoothed
        std::vector<int>   topBin;                                 // the loudest bin of each grid cell (or -1)
        std::vector<float> oct;                                    // octaves from 1 kHz, for the slope
        std::vector<float> broad;                                  // louder channel over 1/3 octave, no slope
        std::vector<float> weight;                                 // W of the chosen weighting at each grid point
        int    weightCurve = -1;  float weightSlope = -1.0f;  double weightSr = 0.0;
    };
    // A peak or a dip of the curve: where it stands out from its neighbourhood, and where its sides cross half
    // of that height (loHz, hiHz).
    struct Feature { double hz = 0.0; float db = 0.0f; float prominence = 0.0f; double loHz = 0.0, hiHz = 0.0; };
    // A stretch of the broad curve that lies away from the line: devDb is its average distance (+ above).
    struct Stretch { double loHz = 0.0, hiHz = 0.0; float devDb = 0.0f; bool fromBottom = false, toTop = false; };
    struct A11yShape
    {
        bool  lineValid = false;  float lineDb = 0.0f;         // the line's level, in dB with the slope (flat on the plot)
        bool  tiltValid = false;  float tiltDbPerOct = 0.0f;   // the signal's own tilt, without the slope
        std::vector<Stretch> stretches;                        // the shape against the line, low to high
        std::vector<Feature> peaks, dips;
        int   totalPeaks = 0, totalDips = 0;   // how many stand out, before the list is cut
        float threshold = 3.0f;   // how far a stretch or a feature must stand out to count
        float weightTilt = 0.0f;  // how the weighting itself leans from 50 Hz to 10 kHz (= the slope with Slope)
    };
    static constexpr int kMaxPeaks = 10, kMaxDips = 10;

    mutable A11yCurve curve;
    mutable A11yShape shape;
    mutable juce::uint32 curveFrame = 0xffffffffu, shapeFrame = 0xffffffffu;
    mutable int    curveSmooth = -1, shapeSmooth = -1, shapeRange = -1;
    mutable float  shapeSlope = -1.0f;
    mutable int    shapeWeighting = -1;
    mutable juce::uint32 shapeTimeMs = 0;
    mutable bool   shapeHadFrame = false;

    const A11yCurve& a11yCurve() const;   // rebuilt at most once per frame
    const A11yShape& a11yShape() const;   // the list holds still for a second, so rows read in a row agree
    float  a11yCurveAt (const std::vector<float>& v, double hz) const;
    float  a11yProbeLevel (int slot, double hz, bool hold) const;
    juce::String a11yLevels (double hz, bool withHold) const;    // "L -18 dB, R -20 dB"
    juce::String a11yLineDeviation (double hz) const;            // "3 dB above the line"
    juce::String a11yTiltValue() const;                          // "falls 0.5 dB per octave, darker than the line"
    juce::String a11yShapeValue() const;                         // "line at -40 dB; 35 to 80 Hz +5 dB, 85 to 400 Hz -4 dB"
    juce::String stretchesText (const std::vector<Stretch>& stretches) const;
    static std::vector<Stretch> findStretches (const std::vector<float>& dev, float threshold);

    juce::String a11yFeatureValue (const Feature& f, bool dip) const;
    juce::String a11yBandValue (int band) const;
    juce::String a11yControlValue (int item) const;
    static juce::String a11yWeightingName (int curve);
    juce::String a11yMaskingValue() const;            // "masked: 700 Hz to 1.9 kHz; below hearing: above 15 kHz"
    juce::String a11yMaskAt (double hz) const;        // "masked", "below hearing" or empty
    int          a11yFeatureRows() const;
    int          a11ySettingItem (int row) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumLens)
};
}
